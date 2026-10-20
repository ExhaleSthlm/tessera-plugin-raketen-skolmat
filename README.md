# Raketen skolmat

Privat Tessera-plugin för dagens skolmat, anpassat för en 1×2-tile på Guition 4".

Lägg till `https://github.com/ExhaleSthlm/tessera-plugin-raketen-skolmat` som eget plugin i Tessera, bygg skärmens firmware och lägg till **Skolmat** i layouten. Ingen API-nyckel behövs.

Två måltidsalternativ växlar var tolfte sekund. Tryck för att öppna hela den hämtade veckans meny i en rullbar helskärmsvy. På lördag och söndag visas kommande måndags meny om den finns publicerad, annars ”Måndagens meny saknas”. Övriga dagar utan meny visar ”Ingen matsedel för idag”.

Tilen kan dras från 1×1 till 2×2. Tessera skriver storlekar som bredd × höjd: 2×1 är bred, 1×2 är hög. Uppdatera pluginet i Tessera och bygg firmware efter en pluginuppdatering.

GitHub Actions hämtar skolans HTML dagligen 03:17 UTC (05:17 sommartid, 04:17 vintertid) och sparar ändringar i `data/menu.json`. Kan också köras manuellt under **Actions → Hämta matsedel**. Tessera/HA läser JSON varje timme. Efter första pushen startar hämtningen automatiskt.

## Privacy

HA hämtar offentlig menydata från GitHub, som ser anslutningens IP-adress. Pluginet skickar inga entiteter, personuppgifter eller API-nycklar.
