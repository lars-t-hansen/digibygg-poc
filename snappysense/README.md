# SnappySense prototyp 2022/2023

En reboot av SnappySense-prosjektet for MQTT og AWS IoT / AWS Lambda anno 2022.

## Status pr 2022-12-14

- Det er et greit design på plass, se `design.md`, `data-model.md` og `mqtt-protocol.md` for
  henholdsvis overordnet design, beskrivelse av data i DynamoDB, og beskrivelse av protokollen
  via MQTT for kommunikasjon mellom device og server.

- Klient-koden er operasjonell (med simulert sensor pr nå), denne kjører på Linux og bruker lite
  ressurser, se `mqtt-client/README.md`.  Denne vil være helt OK for sensorer og aktuatorer som
  kjører på embedded Linux, i hvert fall.

- Server-koden for klient-kommunikasjon (mot AWS Lambda med DynamoDB) er operasjonell, mer eller
  mindre (testet lokalt), se `lambda/README.md`.  Den håndterer mottak av sensorlesinger og sender
  meldinger tilbake til aktuatorer.

- Server-koden for REST-API for web server er under utarbeidelse.
