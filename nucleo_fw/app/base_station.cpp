#include "base_station.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace app {

namespace {

struct Rendered {
    std::uint8_t severity;
    std::array<char, 50> text;  // STATUSTEXT.text is exactly 50 bytes; pack() copies all 50
};

Rendered render(const tp::AlertEvent& e) noexcept {
    Rendered r{MAV_SEVERITY_INFO, {}};   // zero-filled, so the string is always NUL-terminated
    const int pct = static_cast<int>(e.probability * 100.0f + 0.5f);
    switch (e.kind) {
        case tp::AlertKind::DropPredicted:
            r.severity = MAV_SEVERITY_WARNING;
            std::snprintf(r.text.data(), r.text.size(), "DROP PREDICTED p=%d%%", pct);
            break;
        case tp::AlertKind::DropCleared:
            std::snprintf(r.text.data(), r.text.size(), "drop risk cleared");
            break;
        case tp::AlertKind::LinkLost:
            r.severity = MAV_SEVERITY_CRITICAL;
            std::snprintf(r.text.data(), r.text.size(), "LINK LOST");
            break;
        case tp::AlertKind::LinkUp:
            std::snprintf(r.text.data(), r.text.size(), "link up");
            break;
    }
    return r;
}

}  // namespace

void BaseStation::on_bytes(std::span<const std::uint8_t> data, std::uint32_t now_ms) noexcept {
    mavlink_message_t msg;
    for (const std::uint8_t byte : data) {
        if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &parse_status_) == 0) continue;
        if (msg.sysid == kSysId) continue;                 // ignore our own echoes
        pipeline_.on_packet(now_ms, msg.seq);
        if (msg.msgid != MAVLINK_MSG_ID_NAMED_VALUE_FLOAT) continue;

        mavlink_named_value_float_t nv;
        mavlink_msg_named_value_float_decode(&msg, &nv);
        // name[10] is NOT guaranteed NUL-terminated
        const std::string_view name{nv.name, strnlen(nv.name, sizeof nv.name)};
        if (name == "SENSOR") pipeline_.on_sample(nv.time_boot_ms, nv.value);
    }
}

std::optional<std::size_t> BaseStation::poll(std::uint32_t now_ms,
                                             std::span<std::uint8_t> out) noexcept {
    if (out.size() < MAVLINK_MAX_PACKET_LEN) return std::nullopt;
    const auto event = pipeline_.poll(now_ms);
    if (!event) return std::nullopt;

    const Rendered r = render(*event);
    mavlink_message_t m;
    mavlink_msg_statustext_pack(kSysId, MAV_COMP_ID_ONBOARD_COMPUTER, &m, r.severity,
                                r.text.data(), /*id=*/0, /*chunk_seq=*/0);
    return mavlink_msg_to_send_buffer(out.data(), &m);
}

}  // namespace app
