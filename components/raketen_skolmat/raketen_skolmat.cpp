#include "raketen_skolmat.h"
#include <algorithm>
#include <string>

namespace esphome::raketen_skolmat {
namespace ui = tessera::ui;
using tessera::Font;

class TodayTile : public tessera::Tile {
 public:
  explicit TodayTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    title_ = ui::label(c.parent, Font::TITLE, theme::ACCENT);
    lv_obj_set_width(title_, c.width);
    ui::set_text(title_, plugin_->text("title"));
    date_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(date_, c.width);
    const int date_y = ui::line_height(Font::TITLE) + ui::px(4);
    lv_obj_set_y(date_, date_y);
    footer_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_size(footer_, c.width, ui::line_height(Font::BODY));
    lv_obj_set_y(footer_, c.height - ui::line_height(Font::BODY));
    body_ = ui::text(c.parent, Font::BODY, theme::INK);
    const int top = date_y + ui::line_height(Font::BODY) + ui::px(12);
    lv_obj_set_pos(body_, 0, top);
    lv_obj_set_size(body_, c.width, std::max(1, c.height - top - ui::line_height(Font::BODY) - ui::px(8)));
    ui::set_text(body_, plugin_->text("waiting"));
  }

  void on_state(JsonObjectConst data) override {
    days_.clear();
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      Day day;
      day.start = item["start"] | 0u;
      day.end = item["end"] | 0u;
      for (int i = 0; i < 6; ++i) {
        const std::string key = "t" + std::to_string(i);
        day.text += item[key.c_str()] | "";
      }
      days_.push_back(day);
    }
    stale_ = data["stale"] | false;
    failed_ = std::string(data["wait"] | "") == "failed";
  }

  void on_tick(uint32_t now) override {
    ui::set_text(date_, now ? tessera::date_text(now) : "");
    const Day *today = nullptr;
    for (const auto &day : days_)
      if (now >= day.start && now < day.end) today = &day;
    if (!today) {
      ui::set_text(body_, plugin_->text(!now ? "waiting" : failed_ ? "failed" : "missing"));
      ui::set_text(footer_, "");
      return;
    }
    const std::string text(today->text.c_str());
    const size_t split = text.find("\n\n");
    const bool second = split != std::string::npos && ((now / 12 + offset_) % 2);
    ui::set_text(body_, split == std::string::npos ? text : second ? text.substr(split + 2) : text.substr(0, split));
    ui::set_text(footer_, stale_ ? plugin_->text("stale") : split == std::string::npos ? "" : plugin_->text(second ? "second" : "first"));
  }

  void on_tap() override { ++offset_; on_tick(tessera::epoch()); }

  void on_theme() override {
    ui::set_color(title_, theme::ACCENT);
    ui::set_color(date_, theme::MUTED);
    ui::set_color(body_, theme::INK);
    ui::set_color(footer_, theme::MUTED);
  }

 private:
  struct Day { uint32_t start = 0, end = 0; tessera::Text text; };
  const tessera::Plugin *plugin_;
  tessera::List<Day> days_;
  lv_obj_t *title_{}, *date_{}, *body_{}, *footer_{};
  unsigned offset_ = 0;
  bool stale_ = false, failed_ = false;
};

void RaketenSkolmat::setup() {
  add_tile("today", [this]() { return new TodayTile(this); });
}
}  // namespace esphome::raketen_skolmat
