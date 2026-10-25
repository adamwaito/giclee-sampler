#include "giclee/webui/ResourceRouter.h"

#include <algorithm>
#include <cstring>
#include <map>

namespace giclee::webui
{

Resource Resource::fromString (const juce::String& text, juce::String mimeType)
{
    const auto utf8 = text.toRawUTF8();
    const auto size = std::strlen (utf8);

    Resource r;
    r.data.resize (size);
    std::memcpy (r.data.data(), utf8, size);
    r.mimeType = std::move (mimeType);
    return r;
}

Resource Resource::fromJson (const juce::var& value)
{
    return fromString (juce::JSON::toString (value, true), "application/json");
}

Resource Resource::fromFloats (std::span<const float> values)
{
    static_assert (sizeof (float) == 4);

    Resource r;
    r.data.resize (values.size_bytes());

    if (! values.empty())
        std::memcpy (r.data.data(), values.data(), values.size_bytes());

    r.mimeType = "application/octet-stream";
    return r;
}

Request Request::parse (const juce::String& url)
{
    Request req;
    req.path = url.upToFirstOccurrenceOf ("?", false, false);

    const auto queryString = url.fromFirstOccurrenceOf ("?", false, false);

    for (const auto& pair : juce::StringArray::fromTokens (queryString, "&", ""))
    {
        if (pair.isEmpty())
            continue;

        const auto key = juce::URL::removeEscapeChars (pair.upToFirstOccurrenceOf ("=", false, false));
        const auto value = juce::URL::removeEscapeChars (pair.fromFirstOccurrenceOf ("=", false, false));
        req.query.set (key, value);
    }

    return req;
}

void ResourceRouter::addPrefix (const juce::String& prefix, Handler handler)
{
    jassert (prefix.startsWithChar ('/') && handler != nullptr);
    prefixes.emplace_back (prefix, std::move (handler));

    std::stable_sort (prefixes.begin(), prefixes.end(),
                      [] (const auto& a, const auto& b) { return a.first.length() > b.first.length(); });
}

void ResourceRouter::setStaticAssets (const void* zipData, size_t zipSize)
{
    zip.reset();
    zipStream.reset();

    if (zipData == nullptr || zipSize == 0)
        return;

    zipStream = std::make_unique<juce::MemoryInputStream> (zipData, zipSize, false);
    zip = std::make_unique<juce::ZipFile> (*zipStream);
}

std::optional<Resource> ResourceRouter::serve (const juce::String& url) const
{
    const auto request = Request::parse (url);

    for (const auto& [prefix, handler] : prefixes)
        if (request.path.startsWith (prefix))
            return handler (request);

    return serveStatic (request.path);
}

std::optional<Resource> ResourceRouter::serveStatic (const juce::String& path) const
{
    if (zip == nullptr)
        return std::nullopt;

    auto name = juce::URL::removeEscapeChars (path).trimCharactersAtStart ("/");

    if (name.isEmpty())
        name = "index.html";

    auto* entry = zip->getEntry (name);

    // Client-side routes (no file extension) get the app shell.
    if (entry == nullptr && ! name.fromLastOccurrenceOf ("/", false, false).containsChar ('.'))
    {
        name = "index.html";
        entry = zip->getEntry (name);
    }

    if (entry == nullptr)
        return std::nullopt;

    std::unique_ptr<juce::InputStream> stream (zip->createStreamForEntry (*entry));

    if (stream == nullptr)
        return std::nullopt;

    Resource r;
    r.data.resize (static_cast<size_t> (entry->uncompressedSize));

    if (! r.data.empty())
    {
        const auto bytesRead = stream->read (r.data.data(), r.data.size());

        if (bytesRead < 0 || static_cast<size_t> (bytesRead) != r.data.size())
            return std::nullopt;
    }

    r.mimeType = mimeTypeForPath (name);
    return r;
}

juce::String ResourceRouter::mimeTypeForPath (const juce::String& path)
{
    static const std::map<juce::String, juce::String> types {
        { "html",  "text/html" },
        { "htm",   "text/html" },
        { "js",    "text/javascript" },
        { "mjs",   "text/javascript" },
        { "css",   "text/css" },
        { "json",  "application/json" },
        { "map",   "application/json" },
        { "svg",   "image/svg+xml" },
        { "png",   "image/png" },
        { "jpg",   "image/jpeg" },
        { "jpeg",  "image/jpeg" },
        { "webp",  "image/webp" },
        { "gif",   "image/gif" },
        { "ico",   "image/vnd.microsoft.icon" },
        { "woff",  "font/woff" },
        { "woff2", "font/woff2" },
        { "ttf",   "font/ttf" },
        { "otf",   "font/otf" },
        { "wasm",  "application/wasm" },
        { "wav",   "audio/wav" },
        { "txt",   "text/plain" },
    };

    const auto file = path.fromLastOccurrenceOf ("/", false, false);

    if (! file.containsChar ('.'))
        return "application/octet-stream";

    const auto ext = file.fromLastOccurrenceOf (".", false, false).toLowerCase();

    if (const auto it = types.find (ext); it != types.end())
        return it->second;

    return "application/octet-stream";
}

} // namespace giclee::webui
