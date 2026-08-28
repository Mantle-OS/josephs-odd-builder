#pragma once

#include <cstdint>
#include <string_view>

namespace job::net {

enum class JobHttpStatusCode : std::uint16_t
{
    Unknown = 0,

    // =========================================================================
    // 1xx - Informational
    // =========================================================================
    Continue                  = 100,
    SwitchingProtocols       = 101,
    Processing                = 102,
    EarlyHints                = 103,
    UploadResumptionSupported = 104, // Temporary IANA registration.

    // =========================================================================
    // 2xx - Success
    // =========================================================================
    Ok                          = 200,
    Created                     = 201,
    Accepted                    = 202,
    NonAuthoritativeInformation = 203,
    NoContent                   = 204,
    ResetContent                = 205,
    PartialContent              = 206,
    MultiStatus                 = 207,
    AlreadyReported             = 208,
    ImUsed                      = 226,

    // =========================================================================
    // 3xx - Redirection
    // =========================================================================
    MultipleChoices   = 300,
    MovedPermanently  = 301,
    Found              = 302,
    SeeOther           = 303,
    NotModified        = 304,
    UseProxy           = 305,
    TemporaryRedirect  = 307,
    PermanentRedirect  = 308,

    // =========================================================================
    // 4xx - Client Error
    // =========================================================================
    BadRequest                     = 400,
    Unauthorized                   = 401,
    PaymentRequired                = 402,
    Forbidden                      = 403,
    NotFound                       = 404,
    MethodNotAllowed               = 405,
    NotAcceptable                  = 406,
    ProxyAuthenticationRequired    = 407,
    RequestTimeout                 = 408,
    Conflict                       = 409,
    Gone                           = 410,
    LengthRequired                 = 411,
    PreconditionFailed             = 412,
    ContentTooLarge                = 413,
    UriTooLong                     = 414,
    UnsupportedMediaType           = 415,
    RangeNotSatisfiable            = 416,
    ExpectationFailed              = 417,
    MisdirectedRequest             = 421,
    UnprocessableContent           = 422,
    Locked                         = 423,
    FailedDependency               = 424,
    TooEarly                       = 425,
    UpgradeRequired                = 426,
    PreconditionRequired           = 428,
    TooManyRequests                = 429,
    RequestHeaderFieldsTooLarge    = 431,
    UnavailableForLegalReasons     = 451,

    // =========================================================================
    // 5xx - Server Error
    // =========================================================================
    InternalServerError           = 500,
    NotImplemented                = 501,
    BadGateway                    = 502,
    ServiceUnavailable            = 503,
    GatewayTimeout                = 504,
    HttpVersionNotSupported       = 505,
    VariantAlsoNegotiates         = 506,
    InsufficientStorage           = 507,
    LoopDetected                  = 508,
    NotExtended                   = 510, // Obsolete, but still present in the IANA registry.
    NetworkAuthenticationRequired = 511
};

[[nodiscard]] constexpr std::uint16_t toStatusCode(JobHttpStatusCode status) noexcept
{
    return static_cast<std::uint16_t>(status);
}

[[nodiscard]] constexpr std::string_view toString(JobHttpStatusCode status) noexcept
{
    switch (status) {
    case JobHttpStatusCode::Continue:
        return "Continue";
    case JobHttpStatusCode::SwitchingProtocols:
        return "Switching Protocols";
    case JobHttpStatusCode::Processing:
        return "Processing";
    case JobHttpStatusCode::EarlyHints:
        return "Early Hints";
    case JobHttpStatusCode::UploadResumptionSupported:
        return "Upload Resumption Supported";

    case JobHttpStatusCode::Ok:
        return "OK";
    case JobHttpStatusCode::Created:
        return "Created";
    case JobHttpStatusCode::Accepted:
        return "Accepted";
    case JobHttpStatusCode::NonAuthoritativeInformation:
        return "Non-Authoritative Information";
    case JobHttpStatusCode::NoContent:
        return "No Content";
    case JobHttpStatusCode::ResetContent:
        return "Reset Content";
    case JobHttpStatusCode::PartialContent:
        return "Partial Content";
    case JobHttpStatusCode::MultiStatus:
        return "Multi-Status";
    case JobHttpStatusCode::AlreadyReported:
        return "Already Reported";
    case JobHttpStatusCode::ImUsed:
        return "IM Used";

    case JobHttpStatusCode::MultipleChoices:
        return "Multiple Choices";
    case JobHttpStatusCode::MovedPermanently:
        return "Moved Permanently";
    case JobHttpStatusCode::Found:
        return "Found";
    case JobHttpStatusCode::SeeOther:
        return "See Other";
    case JobHttpStatusCode::NotModified:
        return "Not Modified";
    case JobHttpStatusCode::UseProxy:
        return "Use Proxy";
    case JobHttpStatusCode::TemporaryRedirect:
        return "Temporary Redirect";
    case JobHttpStatusCode::PermanentRedirect:
        return "Permanent Redirect";

    case JobHttpStatusCode::BadRequest:
        return "Bad Request";
    case JobHttpStatusCode::Unauthorized:
        return "Unauthorized";
    case JobHttpStatusCode::PaymentRequired:
        return "Payment Required";
    case JobHttpStatusCode::Forbidden:
        return "Forbidden";
    case JobHttpStatusCode::NotFound:
        return "Not Found";
    case JobHttpStatusCode::MethodNotAllowed:
        return "Method Not Allowed";
    case JobHttpStatusCode::NotAcceptable:
        return "Not Acceptable";
    case JobHttpStatusCode::ProxyAuthenticationRequired:
        return "Proxy Authentication Required";
    case JobHttpStatusCode::RequestTimeout:
        return "Request Timeout";
    case JobHttpStatusCode::Conflict:
        return "Conflict";
    case JobHttpStatusCode::Gone:
        return "Gone";
    case JobHttpStatusCode::LengthRequired:
        return "Length Required";
    case JobHttpStatusCode::PreconditionFailed:
        return "Precondition Failed";
    case JobHttpStatusCode::ContentTooLarge:
        return "Content Too Large";
    case JobHttpStatusCode::UriTooLong:
        return "URI Too Long";
    case JobHttpStatusCode::UnsupportedMediaType:
        return "Unsupported Media Type";
    case JobHttpStatusCode::RangeNotSatisfiable:
        return "Range Not Satisfiable";
    case JobHttpStatusCode::ExpectationFailed:
        return "Expectation Failed";
    case JobHttpStatusCode::MisdirectedRequest:
        return "Misdirected Request";
    case JobHttpStatusCode::UnprocessableContent:
        return "Unprocessable Content";
    case JobHttpStatusCode::Locked:
        return "Locked";
    case JobHttpStatusCode::FailedDependency:
        return "Failed Dependency";
    case JobHttpStatusCode::TooEarly:
        return "Too Early";
    case JobHttpStatusCode::UpgradeRequired:
        return "Upgrade Required";
    case JobHttpStatusCode::PreconditionRequired:
        return "Precondition Required";
    case JobHttpStatusCode::TooManyRequests:
        return "Too Many Requests";
    case JobHttpStatusCode::RequestHeaderFieldsTooLarge:
        return "Request Header Fields Too Large";
    case JobHttpStatusCode::UnavailableForLegalReasons:
        return "Unavailable For Legal Reasons";

    case JobHttpStatusCode::InternalServerError:
        return "Internal Server Error";
    case JobHttpStatusCode::NotImplemented:
        return "Not Implemented";
    case JobHttpStatusCode::BadGateway:
        return "Bad Gateway";
    case JobHttpStatusCode::ServiceUnavailable:
        return "Service Unavailable";
    case JobHttpStatusCode::GatewayTimeout:
        return "Gateway Timeout";
    case JobHttpStatusCode::HttpVersionNotSupported:
        return "HTTP Version Not Supported";
    case JobHttpStatusCode::VariantAlsoNegotiates:
        return "Variant Also Negotiates";
    case JobHttpStatusCode::InsufficientStorage:
        return "Insufficient Storage";
    case JobHttpStatusCode::LoopDetected:
        return "Loop Detected";
    case JobHttpStatusCode::NotExtended:
        return "Not Extended";
    case JobHttpStatusCode::NetworkAuthenticationRequired:
        return "Network Authentication Required";

    case JobHttpStatusCode::Unknown:
        return {};
    }

    return {};
}

[[nodiscard]] constexpr JobHttpStatusCode toHttpStatusCode(std::uint16_t status) noexcept
{
    switch (status) {
    case 100:
        return JobHttpStatusCode::Continue;
    case 101:
        return JobHttpStatusCode::SwitchingProtocols;
    case 102:
        return JobHttpStatusCode::Processing;
    case 103:
        return JobHttpStatusCode::EarlyHints;
    case 104:
        return JobHttpStatusCode::UploadResumptionSupported;

    case 200:
        return JobHttpStatusCode::Ok;
    case 201:
        return JobHttpStatusCode::Created;
    case 202:
        return JobHttpStatusCode::Accepted;
    case 203:
        return JobHttpStatusCode::NonAuthoritativeInformation;
    case 204:
        return JobHttpStatusCode::NoContent;
    case 205:
        return JobHttpStatusCode::ResetContent;
    case 206:
        return JobHttpStatusCode::PartialContent;
    case 207:
        return JobHttpStatusCode::MultiStatus;
    case 208:
        return JobHttpStatusCode::AlreadyReported;
    case 226:
        return JobHttpStatusCode::ImUsed;

    case 300:
        return JobHttpStatusCode::MultipleChoices;
    case 301:
        return JobHttpStatusCode::MovedPermanently;
    case 302:
        return JobHttpStatusCode::Found;
    case 303:
        return JobHttpStatusCode::SeeOther;
    case 304:
        return JobHttpStatusCode::NotModified;
    case 305:
        return JobHttpStatusCode::UseProxy;
    case 307:
        return JobHttpStatusCode::TemporaryRedirect;
    case 308:
        return JobHttpStatusCode::PermanentRedirect;

    case 400:
        return JobHttpStatusCode::BadRequest;
    case 401:
        return JobHttpStatusCode::Unauthorized;
    case 402:
        return JobHttpStatusCode::PaymentRequired;
    case 403:
        return JobHttpStatusCode::Forbidden;
    case 404:
        return JobHttpStatusCode::NotFound;
    case 405:
        return JobHttpStatusCode::MethodNotAllowed;
    case 406:
        return JobHttpStatusCode::NotAcceptable;
    case 407:
        return JobHttpStatusCode::ProxyAuthenticationRequired;
    case 408:
        return JobHttpStatusCode::RequestTimeout;
    case 409:
        return JobHttpStatusCode::Conflict;
    case 410:
        return JobHttpStatusCode::Gone;
    case 411:
        return JobHttpStatusCode::LengthRequired;
    case 412:
        return JobHttpStatusCode::PreconditionFailed;
    case 413:
        return JobHttpStatusCode::ContentTooLarge;
    case 414:
        return JobHttpStatusCode::UriTooLong;
    case 415:
        return JobHttpStatusCode::UnsupportedMediaType;
    case 416:
        return JobHttpStatusCode::RangeNotSatisfiable;
    case 417:
        return JobHttpStatusCode::ExpectationFailed;
    case 421:
        return JobHttpStatusCode::MisdirectedRequest;
    case 422:
        return JobHttpStatusCode::UnprocessableContent;
    case 423:
        return JobHttpStatusCode::Locked;
    case 424:
        return JobHttpStatusCode::FailedDependency;
    case 425:
        return JobHttpStatusCode::TooEarly;
    case 426:
        return JobHttpStatusCode::UpgradeRequired;
    case 428:
        return JobHttpStatusCode::PreconditionRequired;
    case 429:
        return JobHttpStatusCode::TooManyRequests;
    case 431:
        return JobHttpStatusCode::RequestHeaderFieldsTooLarge;
    case 451:
        return JobHttpStatusCode::UnavailableForLegalReasons;

    case 500:
        return JobHttpStatusCode::InternalServerError;
    case 501:
        return JobHttpStatusCode::NotImplemented;
    case 502:
        return JobHttpStatusCode::BadGateway;
    case 503:
        return JobHttpStatusCode::ServiceUnavailable;
    case 504:
        return JobHttpStatusCode::GatewayTimeout;
    case 505:
        return JobHttpStatusCode::HttpVersionNotSupported;
    case 506:
        return JobHttpStatusCode::VariantAlsoNegotiates;
    case 507:
        return JobHttpStatusCode::InsufficientStorage;
    case 508:
        return JobHttpStatusCode::LoopDetected;
    case 510:
        return JobHttpStatusCode::NotExtended;
    case 511:
        return JobHttpStatusCode::NetworkAuthenticationRequired;

    default:
        return JobHttpStatusCode::Unknown;
    }
}

} // namespace job::net