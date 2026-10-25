#pragma once

#include <juce_core/juce_core.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace giclee::webui
{

/** A response body; mirrors juce::WebBrowserComponent::Resource without needing juce_gui_extra. */
struct Resource
{
    std::vector<std::byte> data;
    juce::String mimeType;

    static Resource fromString (const juce::String& text, juce::String mimeType);
    static Resource fromJson (const juce::var& value);

    /** Raw little-endian float32 array (all supported targets are little-endian). */
    static Resource fromFloats (std::span<const float> values);
};

/** A parsed request: "/api/peaks/x?bins=512" → path "/api/peaks/x", query { bins: "512" }. */
struct Request
{
    juce::String path;
    juce::StringPairArray query;

    static Request parse (const juce::String& url);
};

/** Serves the WebView's resource-provider requests.

    "/api/..." style prefixes go to registered handlers, which serve bulk binary data such as
    waveform peaks without JSON overhead. Everything else is served from the embedded frontend
    zip, with "/" mapped to index.html and extension-less paths falling back to index.html.

    Called on the message thread, so handlers must be quick (serve precomputed data).
*/
class ResourceRouter
{
public:
    using Handler = std::function<std::optional<Resource> (const Request&)>;

    /** Longest matching prefix wins. */
    void addPrefix (const juce::String& prefix, Handler handler);

    /** Zip archive of the built frontend. The data must outlive this router (binary data). */
    void setStaticAssets (const void* zipData, size_t zipSize);

    std::optional<Resource> serve (const juce::String& url) const;

    static juce::String mimeTypeForPath (const juce::String& path);

private:
    std::optional<Resource> serveStatic (const juce::String& path) const;

    std::vector<std::pair<juce::String, Handler>> prefixes;
    std::unique_ptr<juce::MemoryInputStream> zipStream;
    std::unique_ptr<juce::ZipFile> zip;
};

} // namespace giclee::webui
