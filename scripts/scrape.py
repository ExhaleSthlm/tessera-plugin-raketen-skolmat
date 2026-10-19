#!/usr/bin/env python3
"""Fetch Raketen's menu; only replace JSON after a complete parse."""
import json
import re
import sys
from datetime import datetime, timedelta
from html.parser import HTMLParser
from pathlib import Path
from urllib.request import Request, urlopen
from zoneinfo import ZoneInfo

URL = "https://kulturskolanraketen.se/matsedel/"
ZONE = ZoneInfo("Europe/Stockholm")
MONTHS = "januari februari mars april maj juni juli augusti september oktober november december".split()
WEEKDAYS = "måndag tisdag onsdag torsdag fredag".split()


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.parts = []
        self.skip = 0

    def handle_starttag(self, tag, attrs):
        if tag in ("script", "style"):
            self.skip += 1
        if tag in ("h2", "h3", "p", "div", "br"):
            self.parts.append("\n")

    def handle_endtag(self, tag):
        if tag in ("script", "style"):
            self.skip = max(0, self.skip - 1)
        if tag in ("h2", "h3", "p", "div"):
            self.parts.append("\n")

    def handle_data(self, data):
        if not self.skip:
            self.parts.append(data)


def parse(html):
    page = Page()
    page.feed(html)
    text = " ".join("".join(page.parts).split())
    section = re.search(r"VECKA\s+(\d{1,2})\b(.*?)SMAKLIG MÅLTID", text, re.I)
    if not section:
        raise ValueError("Hittar inte matsedelns början och slut")
    week, menu = int(section[1]), section[2]
    pattern = r"(" + "|".join(WEEKDAYS) + r")\s+(\d{1,2})\s+(" + "|".join(MONTHS) + r")\b"
    headings = list(re.finditer(pattern, menu, re.I))
    if len(headings) != 5:
        raise ValueError("Förväntade fem daterade skoldagar")
    today = datetime.now(ZONE).date()
    days = []
    for index, heading in enumerate(headings):
        weekday = WEEKDAYS.index(heading[1].lower())
        month, day = MONTHS.index(heading[3].lower()) + 1, int(heading[2])
        candidates = []
        for year in range(today.year - 1, today.year + 2):
            try:
                candidate = datetime(year, month, day, tzinfo=ZONE)
            except ValueError:
                continue
            if candidate.weekday() == weekday and candidate.isocalendar().week == week:
                candidates.append(candidate)
        if not candidates:
            raise ValueError("Datum, veckodag och veckonummer stämmer inte")
        start = min(candidates, key=lambda d: abs((d.date() - today).days))
        stop = headings[index + 1].start() if index + 1 < len(headings) else len(menu)
        body = menu[heading.end():stop].strip()
        meals = [s.strip() for s in re.split(r"ALTERNATIV\s*\d+\s*:\s*", body, flags=re.I) if s.strip()]
        if not meals:
            raise ValueError("Tom måltid")
        # Tessera maps at most 48 UTF-8 bytes per text field. Six chunks
        # preserve full meal text, including Swedish characters.
        chunks = [""]
        for char in "\n\n".join(meals):
            if len((chunks[-1] + char).encode("utf-8")) > 48:
                chunks.append("")
            chunks[-1] += char
        if len(chunks) > 6:
            raise ValueError("Måltidstexten överstiger tile-formatets 288 byte")
        days.append({"date": start.date().isoformat(), "start": int(start.timestamp()),
                     "end": int((start + timedelta(days=1)).timestamp()), "meals": meals,
                     **{f"t{i}": chunks[i] if i < len(chunks) else "" for i in range(6)}})
    if [d["date"] for d in days] != sorted({d["date"] for d in days}):
        raise ValueError("Dagarna är inte unika och i datumordning")
    return {"source": URL, "week": week, "days": days}


if __name__ == "__main__":
    if len(sys.argv) > 1:
        html = Path(sys.argv[1]).read_text()
    else:
        with urlopen(Request(URL, headers={"User-Agent": "RaketenSkolmat/1.0"}), timeout=30) as response:
            html = response.read().decode("utf-8")
    result = parse(html)
    destination = Path(__file__).resolve().parents[1] / "data/menu.json"
    destination.parent.mkdir(exist_ok=True)
    destination.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(f"Sparade vecka {result['week']}: {len(result['days'])} dagar")
