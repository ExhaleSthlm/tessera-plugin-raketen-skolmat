#include "raketen_skolmat.h"
#include <algorithm>
#include <string>

namespace esphome::raketen_skolmat {
namespace ui = tessera::ui;
using tessera::Font;

static std::string meal_text(JsonObjectConst item) {
  std::string text;
  for (int i = 0; i < 6; ++i) {
    const std::string key = "t" + std::to_string(i);
    text += item[key.c_str()] | "";
  }
  return text;
}

class WeekCard : public tessera::Card {
 public:
  explicit WeekCard(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void open(const tessera::CardContext &c) override {
    scroll_ = lv_obj_create(c.parent);
    lv_obj_remove_style_all(scroll_);
    lv_obj_set_size(scroll_, c.width, c.height);
    lv_obj_set_scroll_dir(scroll_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scroll_, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(scroll_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(scroll_, LV_OBJ_FLAG_SCROLLABLE);
    text_ = ui::text(scroll_, Font::BODY, theme::INK);
    lv_obj_set_width(text_, std::max(1, c.width - ui::px(12)));
    lv_label_set_long_mode(text_, LV_LABEL_LONG_WRAP);
    lv_obj_set_height(text_, LV_SIZE_CONTENT);
    ui::set_text(text_, plugin_->text("waiting"));
  }

  void on_state(JsonObjectConst data) override {
    std::string text;
    if (data["stale"] | false) text = std::string(plugin_->text("stale")) + "\n\n";
    bool has_days = false;
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      const uint32_t start = item["start"] | 0u;
      if (!start) continue;
      if (has_days) text += "\n\n";
      text += tessera::date_text(start) + "\n" + meal_text(item);
      has_days = true;
    }
    if (!has_days) text = plugin_->text(std::string(data["wait"] | "") == "failed" ? "failed" : "missing_week");
    ui::set_text(text_, text);
  }

  void on_theme() override { ui::set_color(text_, theme::INK); }

 private:
  const tessera::Plugin *plugin_;
  lv_obj_t *scroll_{}, *text_{};
};

class TodayTile : public tessera::Tile {
 public:
  explicit TodayTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    entity_ = c.entity ? c.entity : "";
    tile_ = c.tile;
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
    const bool compact = c.rows == 1;
    if (compact) lv_obj_add_flag(footer_, LV_OBJ_FLAG_HIDDEN);
    const int top = date_y + ui::line_height(Font::BODY) + ui::px(compact ? 4 : 12);
    lv_obj_set_pos(body_, 0, top);
    lv_obj_set_size(body_, c.width, std::max(1, c.height - top - (compact ? 0 : ui::line_height(Font::BODY) + ui::px(8))));
    ui::set_text(body_, plugin_->text("waiting"));
  }

  void on_state(JsonObjectConst data) override {
    days_.clear();
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      Day day;
      day.start = item["start"] | 0u;
      day.end = item["end"] | 0u;
      day.text = meal_text(item).c_str();
      days_.push_back(day);
    }
    stale_ = data["stale"] | false;
    failed_ = std::string(data["wait"] | "") == "failed";
  }

  void on_tick(uint32_t now) override {
    ui::set_text(date_, now ? tessera::date_text(now) : "");
    const Day *today = nullptr;
    const int weekday = now ? tessera::local_time(now).weekday : 0;
    const int ahead = weekday == 7 ? 2 : weekday == 1 ? 1 : 0;
    for (const auto &day : days_)
      if (ahead ? tessera::days_from_today(day.start) == ahead : now >= day.start && now < day.end) today = &day;
    if (!today) {
      ui::set_text(body_, plugin_->text(!now ? "waiting" : failed_ ? "failed" : ahead ? "missing_monday" : "missing"));
      ui::set_text(footer_, "");
      return;
    }
    ui::set_text(date_, tessera::date_text(today->start));
    const std::string text(today->text.c_str());
    const size_t split = text.find("\n\n");
    const bool second = split != std::string::npos && ((now / 12) % 2);
    ui::set_text(body_, split == std::string::npos ? text : second ? text.substr(split + 2) : text.substr(0, split));
    ui::set_text(footer_, stale_ ? plugin_->text("stale") : split == std::string::npos ? "" : plugin_->text(second ? "second" : "first"));
  }

  void on_tap() override {
    tessera::open_card(plugin_->plugin_id(), "week", entity_.c_str(), tile_, plugin_->text("week"));
  }

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
  tessera::Text entity_;
  int tile_ = -1;
  bool stale_ = false, failed_ = false;
};

void RaketenSkolmat::setup() {
  add_tile("today", [this]() { return new TodayTile(this); });
  add_card("week", [this]() { return new WeekCard(this); }, true);
}
}  // namespace esphome::raketen_skolmat
