# Raketen skolmat

Privat Tessera-plugin för dagens skolmat, anpassat för en 1×2-tile på Guition 4".

Lägg till `https://github.com/ExhaleSthlm/tessera-plugin-raketen-skolmat` som eget plugin i Tessera, bygg skärmens firmware och lägg till **Skolmat** i layouten. Ingen API-nyckel behövs.

Två måltidsalternativ växlar var tolfte sekund; tryck för att växla direkt. Dagar utan meny visar ”Ingen matsedel för idag”.

GitHub Actions hämtar skolans HTML dagligen 03:17 UTC (05:17 sommartid, 04:17 vintertid) och sparar ändringar i `data/menu.json`. Kan också köras manuellt under **Actions → Hämta matsedel**. Tessera/HA läser JSON varje timme. Efter första pushen startar hämtningen automatiskt.

## Privacy

HA hämtar offentlig menydata från GitHub, som ser anslutningens IP-adress. Pluginet skickar inga entiteter, personuppgifter eller API-nycklar.
