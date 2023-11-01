#include "HttpData.h"


// ==========================================================================
// MimeType

const std::unordered_map<std::string, std::string> mime{
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".json", "application/json"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".mp4", "video/mp4"},
    {".mp3", "audio/mp3"},
    {".txt", "text/plain"},
    {".xml", "text/xml"},
    {".htm", "text/html"},
    {".c", "text/plain"},
    {".txt", "text/plain"},
    {"default", "text/html"}
};


std::string MimeType::get_mime_type(const std::string &suffix) {
    if (mime.find(suffix) != mime.end()) {
        return mime.at(suffix);
    }
    return mime.at("default");
}


// ==========================================================================
// HttpData
