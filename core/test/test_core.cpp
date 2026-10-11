// Host-side tests. No framework needed (swap in doctest/Catch2 later if you like).
#include <cmath>
#include <cstdint>
#include <iostream>
#include <source_location>
#include <vector>

#include "../pipeline.hpp"
#include "../model_weights.hpp"
#include "golden.hpp"

namespace {

int g_failures = 0;
void check(bool ok, std::source_location where = std::source_location::current()) {
    if (!ok) {
        std::cout << "FAIL line " << where.line() << "\n";
        ++g_failures;
    }
}
bool near(float a, float b, float tol) { return std::fabs(a - b) < tol; }

std::vector<tp::AlertEvent> drain(tp::Pipeline& p, std::uint32_t now) {
    std::vector<tp::AlertEvent> out;
    while (auto e = p.poll(now)) out.push_back(*e);
    return out;
}
bool contains(const std::vector<tp::AlertEvent>& v, tp::AlertKind k) {
    for (const auto& e : v) if (e.kind == k) return true;
    return false;
}

void features_linear() {
    tp::Window w;
    for (std::uint32_t i = 0; i < 40; ++i) w.push(1000 + i * 100, 50.0f - 2.0f * (static_cast<float>(i) * 0.1f));
    const auto f = w.features(32);
    check(f.has_value());
    check(near((*f)[tp::feat::slope], -2.0f, 1e-3f));
    check(near((*f)[tp::feat::last], 50.0f - 2.0f * 3.9f, 1e-3f));
    check(!w.features(64).has_value());  // more than capacity
    check(!w.features(1).has_value());   // too short for a slope
}

void features_need_enough_samples() {
    tp::Window w;
    for (std::uint32_t i = 0; i < 5; ++i) w.push(i * 100, 1.0f);
    check(!w.features(8).has_value());
    check(w.features(5).has_value());
}

void timestamp_wrap() {
    tp::Window w;
    for (std::uint32_t i = 0; i < 10; ++i) w.push(0xFFFFFFF0u + i * 100u, static_cast<float>(i));
    const auto f = w.features(10);
    check(f.has_value());
    check(near((*f)[tp::feat::slope], 10.0f, 1e-3f));  // 1 unit / 100 ms across the u32 wrap
}

void link_monitor() {
    tp::LinkMonitor m{3000};
    check(!m.is_up(0));
    m.on_packet(1000, 254);
    m.on_packet(1100, 255);
    m.on_packet(1200, 2);  // seq 0 and 1 lost
    check(m.lost() == 2);
    check(m.is_up(3000));
    check(!m.is_up(4300));
}

void debouncer() {
    tp::Debouncer d{3};
    check(!d.update(true));
    check(!d.update(true));
    check(d.update(true) && d.active());
    check(!d.update(false));
    check(!d.update(true));  // streak broken, still active
    check(d.active());
}

void rule_baseline() {
    tp::Features f{};
    f[tp::feat::last] = 95.0f;
    f[tp::feat::slope] = -4.0f;
    check(tp::predict_rule(f, 2.0f, 90.0f));   // 95 - 8 = 87 < 90
    f[tp::feat::slope] = -1.0f;
    check(!tp::predict_rule(f, 2.0f, 90.0f));
}

void queue() {
    tp::FixedQueue<int, 2> q;
    check(q.push(1) && q.push(2) && !q.push(3));  // full: third is dropped
    check(q.pop() == 1);
    check(q.push(4));
    check(q.pop() == 2 && q.pop() == 4 && !q.pop().has_value());
}

void golden_matches_python() {
    tp::Window w;
    for (std::size_t i = 0; i < kGoldenN; ++i) w.push(kGoldenT[i], kGoldenV[i]);
    const auto f = w.features(kGoldenN);
    check(f.has_value());
    const float p = tp::predict_logistic(tp::kModel, *f);
    std::cout << "golden: C++=" << p << "  python=" << kGoldenP << "\n";
    check(near(p, kGoldenP, 1e-3f));
}

// A hand-made model that fires when the slope is negative: lets us test the pipeline logic
// without depending on trained weights.
constexpr tp::Model kSlopeModel{
    .mu = {}, .sd = {{1, 1, 1, 1, 1}}, .w = {{0, 0, -1, 0, 0}},
    .b = 0.0f, .threshold = 0.5f, .window_n = 8};

void pipeline_alerts() {
    tp::Pipeline p{kSlopeModel};
    std::uint32_t t = 0;
    std::uint8_t seq = 0;
    auto step = [&](float v) { p.on_packet(t, seq++); p.on_sample(t, v); t += 100; };

    for (int i = 0; i < 12; ++i) step(100.0f);
    auto ev = drain(p, t);
    check(contains(ev, tp::AlertKind::LinkUp));
    check(!contains(ev, tp::AlertKind::DropPredicted));   // flat signal: no alert

    for (int i = 0; i < 20; ++i) step(100.0f - static_cast<float>(i));   // -10 units/s
    ev = drain(p, t);
    check(contains(ev, tp::AlertKind::DropPredicted));

    for (int i = 0; i < 20; ++i) step(80.0f);                            // flat again
    ev = drain(p, t);
    check(contains(ev, tp::AlertKind::DropCleared));
}

void pipeline_link_loss() {
    tp::Pipeline p{kSlopeModel};
    p.on_packet(1000, 0);
    check(contains(drain(p, 1000), tp::AlertKind::LinkUp));
    check(drain(p, 2000).empty());
    check(contains(drain(p, 5000), tp::AlertKind::LinkLost));
    p.on_packet(6000, 1);
    check(contains(drain(p, 6000), tp::AlertKind::LinkUp));
}

}  // namespace

int main() {
    features_linear(); features_need_enough_samples(); timestamp_wrap(); link_monitor();
    debouncer(); rule_baseline(); queue(); golden_matches_python();
    pipeline_alerts(); pipeline_link_loss();
    if (g_failures) { std::cout << g_failures << " FAILED\n"; return 1; }
    std::cout << "all tests passed\n";
    return 0;
}
