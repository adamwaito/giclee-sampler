#include "giclee/webui/ResourceRouter.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>

using giclee::webui::Request;
using giclee::webui::Resource;
using giclee::webui::ResourceRouter;

namespace
{
    juce::String asString (const Resource& r)
    {
        return juce::String::fromUTF8 (reinterpret_cast<const char*> (r.data.data()), static_cast<int> (r.data.size()));
    }

    /** Builds an in-memory zip shaped like a Vite build. */
    juce::MemoryBlock makeSiteZip()
    {
        juce::ZipFile::Builder builder;
        const auto add = [&] (const char* path, const char* text)
        {
            builder.addEntry (std::make_unique<juce::MemoryInputStream> (text, std::strlen (text), true), 9, path, juce::Time());
        };

        add ("index.html", "<html>shell</html>");
        add ("assets/index-abc.js", "console.log(1)");
        add ("assets/style.css", "body{}");

        juce::MemoryOutputStream out;
        builder.writeToStream (out, nullptr);
        return out.getMemoryBlock();
    }
}

TEST_CASE ("requests split into path and decoded query")
{
    const auto r = Request::parse ("/api/peaks/sample%201?bins=512&from=0&name=a%20b&flag");
    CHECK (r.path == "/api/peaks/sample%201");
    CHECK (r.query["bins"] == "512");
    CHECK (r.query["from"] == "0");
    CHECK (r.query["name"] == "a b");
    CHECK (r.query.containsKey ("flag"));

    CHECK (Request::parse ("/plain").query.size() == 0);
}

TEST_CASE ("the longest matching prefix handles the request")
{
    ResourceRouter router;
    router.addPrefix ("/api/", [] (const Request&) { return Resource::fromString ("generic", "text/plain"); });
    router.addPrefix ("/api/peaks/", [] (const Request& r) { return Resource::fromString ("peaks:" + r.query["bins"], "text/plain"); });

    CHECK (asString (*router.serve ("/api/peaks/x?bins=8")) == "peaks:8");
    CHECK (asString (*router.serve ("/api/other")) == "generic");
    CHECK_FALSE (router.serve ("/elsewhere").has_value());
}

TEST_CASE ("static assets are served from the embedded zip")
{
    const auto zip = makeSiteZip();
    ResourceRouter router;
    router.setStaticAssets (zip.getData(), zip.getSize());

    SECTION ("root maps to index.html")
    {
        const auto r = router.serve ("/");
        REQUIRE (r.has_value());
        CHECK (asString (*r) == "<html>shell</html>");
        CHECK (r->mimeType == "text/html");
    }

    SECTION ("files keep their MIME type")
    {
        const auto js = router.serve ("/assets/index-abc.js");
        REQUIRE (js.has_value());
        CHECK (asString (*js) == "console.log(1)");
        CHECK (js->mimeType == "text/javascript");
        CHECK (router.serve ("/assets/style.css")->mimeType == "text/css");
    }

    SECTION ("client-side routes fall back to the shell; missing files don't")
    {
        CHECK (asString (*router.serve ("/review/grid")) == "<html>shell</html>");
        CHECK_FALSE (router.serve ("/assets/missing.js").has_value());
    }
}

TEST_CASE ("float arrays are served as raw bytes")
{
    const float values[] { 1.0f, -0.5f, 0.25f };
    const auto r = Resource::fromFloats (values);
    REQUIRE (r.data.size() == sizeof (values));
    CHECK (std::memcmp (r.data.data(), values, sizeof (values)) == 0);
    CHECK (r.mimeType == "application/octet-stream");
}

TEST_CASE ("MIME types by extension")
{
    CHECK (ResourceRouter::mimeTypeForPath ("a/b/c.WOFF2") == "font/woff2");
    CHECK (ResourceRouter::mimeTypeForPath ("x.json") == "application/json");
    CHECK (ResourceRouter::mimeTypeForPath ("dir.v2/noext") == "application/octet-stream");
    CHECK (ResourceRouter::mimeTypeForPath ("x.unknown") == "application/octet-stream");
}
