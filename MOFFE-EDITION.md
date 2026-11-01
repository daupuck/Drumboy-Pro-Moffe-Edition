# Drumboy Pro – Moffe Edition

Moffe Edition er en videreutviklet utgave av Drumboy Pro-firmwaren fra Randomwaves
(fabrikkversjon v1.01). Denne siden samler alt som er nytt sammenlignet med fabrikkversjonen:
hva funksjonene gjør og hvordan du bruker dem på enheten.

- **Gjeldende versjon:** v1.06
- **Brukermanual (Randomwaves):** [02 Software/drumboy-pro-user-manual.pdf](<02 Software/drumboy-pro-user-manual.pdf>) –
  sidetallene nedenfor viser til den.
- **Tekniske detaljer per versjon:** [CHANGELOG.md](CHANGELOG.md)
- **Bygge, teste og installere:** [02 Software/DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>)

> **Status (1. november 2026):** alt er bygget, enhetstestet og kjørt gjennom CI, men ennå ikke
> testet på en fysisk Drumboy Pro. Ta sikkerhetskopi av SD-kortet før du installerer.

## Knappenumrene

Knappene nummereres slik skjermen viser dem: to rader med fire, fra venstre mot høyre. Navnet på
hver funksjon står på skjermen rett over verdien.

```
 Øverst:   [1]  [2]  [3]  [4]
 Nederst:  [5]  [6]  [7]  [8]
```

---

## 1. Plassere beats nøyaktig – Layer Song-menyen (v1.03)

Åpnes med **S + lagtast** (manualen s. 37–38). Før kunne beats bare lages med mønstergeneratoren
eller spilles inn live, og de kunne ikke flyttes fritt. Nå bruker de to ledige knappene nede til
høyre til å flytte beats.

| Kontroll | Hva den gjør |
|---|---|
| **Knapp 7 – MOVE** | Flytter valgt beat ett steg til venstre eller høyre. Hopper over andre beats, beholder fillen og stopper ved start og slutt av sangen. Under knappen står posisjonen som `takt.slag.sekstendedel`, for eksempel `2.3.4`. En `+` bak betyr at beaten ligger mellom to sekstendedeler. |
| **Knapp 8 – SHIFT ALL** | Flytter *alle* beats på laget ett steg. Det som skyves ut på slutten, kommer inn igjen på starten, så du kommer tilbake ved å vri motsatt vei. Under knappen står steglengden, for eksempel `1:16`. |
| **ADD (kort trykk)** | Setter inn én beat midt i luken etter valgt beat og velger den, så du kan finplassere den med MOVE. På et tomt lag havner den første beaten på starten. |
| ADD (holdes inne, tomt lag) | Mønstergeneratoren, som før. |
| ADD + LEFT/RIGHT | Flytter valgt beat på samme måte som MOVE. |

**Steglengden** er kvantiseringen i Rhythm-menyen (fra 1:64 til 1:1).

**Arbeidsflyt:** velg en beat med LEFT/RIGHT, trykk kort på **ADD**, og vri **MOVE** til beaten
står der du vil.

## 2. Moffe Edition-oppstartsskjerm (v1.04)

Når enheten starter, toner logoen inn, og **pikselsolbriller** glir ned over øynene. Deretter
kommer **MOFFE** frem i store magenta bokstaver, én og én, og **EDITION** toner inn under.

- Tegnes av firmwaren, så SD-kortet og logofilen er uendret, og det virker med alle kort.
- Oppstarten blir ikke lengre, fordi animasjonen bruker pausen som allerede fantes etter logoen.
- Navn, farge og timing er konstanter (`kEdition*`) i appens `Global.h`.

## 3. Forhåndslytting av samples – Layer Instrument-menyen (v1.05)

Åpnes med **I + lagtast** (manualen, Layer Instrument). Nå kan du høre et sample *før* du laster
det.

| Du gjør | Det skjer |
|---|---|
| Vrir **SAMPLE** (eller **INST**) og stopper | Etter et kort øyeblikk (0,15 s) spilles det valgte samplet én gang. Vrir du videre, stopper det, og det neste spilles når du stopper igjen. |
| Trykker **lagtasten** til laget du redigerer | Forhåndslyttingen spilles på nytt så lenge valgt sample er et annet enn det lastede. |
| Trykker **ADD** | Samplet lastes inn på laget, som før. |

- Det som er lastet på laget, røres ikke før du trykker **ADD**.
- **PREVIEW** vises på skjermen der **LOADING** vanligvis står.
- Lyden er tørr, uten lagets pitch, EQ og effekter, men master volume, mute og limiter virker.
- Lange samples starter med en gang, og menyene låses ikke mens filen leses.
- Under innspilling spiller lagtasten det lastede samplet, som før.

**Innstillinger i Metronome-menyen:**

| Knapp | Innstilling | Standard |
|---|---|---|
| **5 – PREVIEW** | Forhåndslytting av eller på | ON |
| **6 – PREV LEVEL** | Lydnivå for forhåndslyttingen, 0–100 | 70 |

Begge lagres og huskes når du slår av og på.

## 4. Euclid-rytmer – EUCLID-siden i Layer Song (v1.06)

Et Euclid-mønster fordeler et antall **slag** så jevnt som mulig over et antall **steg**. Det
er et raskt og musikalsk utgangspunkt for trommer. Eksempler: E(3,8) = `x..x..x.` (tresillo),
E(5,8) = `x.xx.xx.` (cinquillo) og E(4,16) = fire jevne slag.

**Åpne siden:** trykk **S + lagtast** som vanlig, og trykk **S + samme lagtast én gang til**.
Gjør det samme for å gå tilbake.

```
 Øverst:   [1 STEPS]   [2 HITS]   [ stegrute: ■··■··■· ]
 Nederst:  [5 ROTATE]  [6 GRID]   [7 LENGTH]  [8 MODE]
```

| Knapp | Hva den gjør |
|---|---|
| **1 STEPS** | Antall steg i mønsteret (1–32) |
| **2 HITS** | Antall slag (0 til antall steg) |
| **5 ROTATE** | Forskyver mønsteret ett steg om gangen og går rundt |
| **6 GRID** | Lengden på hvert steg, fra 1:32 til 1:4 |
| **7 LENGTH** | **REPEAT** gjentar mønsteret over hele sangen, **ONCE** legger det én gang |
| **8 MODE** | **REPLACE** erstatter lagets beats, **MERGE** legger slagene oppå beatene du har fra før, og de beholder fillene sine |
| **CLEAR** | Går tilbake til beatene laget hadde da du åpnet siden |

- **Du hører endringene med en gang.** Mønsteret skrives til laget hver gang du vrir, så du kan
  justere mens sequenceren spiller.
- **Stegruten** øverst til høyre viser mønsteret. Steget som spilles akkurat nå, markeres i cyan.
- **Polyrytmer:** STEPS 7 med GRID 1:16 gir et mønster på 7/16 som glir mot takten.
- **Maks 64 beats** per lag og bank. Blir mønsteret lengre, vises *MAX 64 BEATS*, og resten
  kuttes.
- **Mønsteret lagres som vanlige beats** i prosjektet, og du kan finjustere det på den vanlige
  siden med MOVE og fills. Knappeinnstillingene på EUCLID-siden lagres ikke.
- **På EUCLID-siden gjør UP/DOWN og et kort ADD-trykk ingenting**, fordi neste vri ville
  overskrevet endringene.

---

## Forbedringer og feilrettinger

### Tryggere firmwareoppdatering (v1.02)
- Bootloaderen starter ikke en ødelagt app etter en mislykket oppdatering. Den viser
  *NO VALID APPLICATION* og prøver pakken igjen ved neste oppstart.
- Feil under skriving til flash vises som *FLASH-CHECK*. Tidligere sto det misvisende *FORMAT-CHECK*.
- **Merk:** bootloaderen kan bare oppdateres med ST-Link (SWD). En SD-oppdatering oppdaterer bare
  appen. Uten ST-Link fortsetter enheten med fabrikk-bootloaderen, og det er trygt.

### Lagring på SD-kortet (v1.02)
- Er kortet fullt, eller feiler en lagring, får du *save error* i stedet for en halvskrevet fil.
- Systeminnstillinger og knappeoppsett som ikke ble lagret, prøves lagret igjen automatisk.
- Instrumentnavn over 25 tegn virker.
- Veldig korte eller ødelagte WAV-filer får ikke lenger enheten til å henge.

### Rettet visning og oppførsel
- Tempo og MIDI-kanal i Global-menyen kunne vises feil (v1.02).
- ADD + LEFT/RIGHT i Layer Song slettet fillen på beaten og på beaten foran (v1.03).
- Å tømme et lag fra en annen meny, for eksempel ved paste, kunne bytte menytilstanden stille (v1.03).
- Etter kvantisering kunne siste beat havne i feil rekkefølge (v1.03).
- I Global-menyen viste REVERB DAMPING og PREDELAY hverandres navn (v1.03).

---

## Versjoner

| Versjon | Innhold |
|---|---|
| v1.01 | Fabrikkfirmware fra Randomwaves (`02 Software/Firmware/Drumboy_Pro_Fw_01_01.bin`) |
| v1.02 | Tryggere firmwareoppdatering, sikrere lagring på SD, feilrettinger |
| v1.03 | Layer Song: MOVE, SHIFT ALL og ADD for én beat |
| v1.04 | Moffe Edition-oppstartsskjerm |
| v1.05 | Forhåndslytting av samples i Layer Instrument-menyen |
| v1.06 | EUCLID-siden i Layer Song: Euclid-rytmer |

## Installere og gå tilbake

1. Ta sikkerhetskopi av hele SD-kortet. `System/File`, `System/Drumkit` og `System/Preset`
   inneholder dine egne data.
2. Legg pakken `Drumboy_Pro_Fw_01_06.bin` i **roten** av kortet. Ikke gi filen nytt navn, og ikke
   endre noe annet på kortet.
3. Sett kortet i enheten og slå den på. Skjermen viser *UPDATING FIRMWARE V1.06* og deretter
   *FIRMWARE UPDATED*.

**Gå tilbake:** legg en eldre pakke i roten på samme måte. Fabrikkversjonen ligger i
`02 Software/Firmware/Drumboy_Pro_Fw_01_01.bin`. Bootloaderen godtar alle versjoner.

Pakken bygges med `cmake --build --preset firmware` (havner i `build/firmware/`), eller lastes
ned fra GitHub Actions under hver kjøring. Se [DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>).

## For utviklere

- Bygging fra kommandolinjen og CI som bygger, tester og verifiserer hver PR.
- Enhetstester på PC for logikken som ikke trenger maskinvare (`02 Software/Tests`).
- `make_fw_package.py` lager og verifiserer SD-pakker etter de samme reglene som bootloaderen.
- `Controller.cpp` er delt opp etter delsystem. Lydmotoren ligger i `ControllerAudio.cpp`.
- Regler og arbeidsflyt for trygge endringer står i [DEVELOPMENT.md](<02 Software/DEVELOPMENT.md>).
