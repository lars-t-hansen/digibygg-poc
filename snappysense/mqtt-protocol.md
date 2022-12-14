-*- fill-column: 100 -*-

# SnappySense MQTT protocol

## Client

Vi har et antall devices som kan måle miljøfaktorer.  Data sendes pr MQTT fra devicet til et
endpoint på AWS IoT.  Hver device har en fast konfigurasjon som inneholder aksesspunkt, device ID,
device klasse, oppdateringsfrekvens, og mange andre ting, bl.a. sertifikater og andre hemmeligheter.
Endringer i konfigurasjon kan sendes til devicet men vil ikke nødvendigvis bli persistert der.

Ved oppstart sender devicet en melding snappy/startup/<device-class>/<device-id> med en JSON-pakke:

```
  { time: <integer, seconds since epoch>,
    reading_interval: <integer, seconds between readings> }
```

hvor reading_interval bare blir sendt ved om devicet er en sensor.  Ved oppstart er devicet normalt
On, dvs, det vil sende observasjoner om det ikke får beskjed om annet.

Ved observasjon sender devicet en melding med topic snappy/reading/<device-class>/<device-id>:

```
  { time: <integer, seconds since epoch>,
    ... }
```

I denne pakken kan devicet sende med felter som representerer siste avlesing av de sensorer som
måtte finnes på devicet, bl.a. disse:

```
  temperature: <number, fractional degrees celsius>,
  humidity: <number, relative in interval 0..1>,
  co2: <number, unit tbd>,
  light: <number, unit tbd>,
```

AWS IoT kan sende kontrollmeldinger til devicet.  Disse sendes til topic snappy/control/<device-id>,
snappy/control/<device-class> og snappy/control/all, som devicet kan abonnere på.  Device-class
indikerer typen device.

Kontrolldataene kan være blant disse:

```
  enable: <number, 0 | 1>,
  reading_interval: <number, positive number of seconds between readings>,
```

hvor enable kontrollerer om devicet utfører og rapporterer målinger, og reading_interval
kontrollerer hvor ofte målingene skjer.

AWS IoT kan også sende kommando-meldinger til devicet.  Disse sendes til snappy/command/<device-id>
(og ikke til verken device-class eller til all, det gir antakelig ikke mening) og kan inneholde
meldinger som disse:

```
  { actuator: <string>, reading: <value>, ideal: <value> }
```

Meningen her er at den navngitte actuator, om den finnes, skal slå inn på en sånn måte at
forskjellen mellom reading og ideal blir mindre.  (Her er det rom for å tenke annerledes / mer.)

Hver klient har en unik device ID (som bare har betydning for applikasjonen) og tilhører en device
klasse (ditto).

Hver klient skal ha et sertifikat med en AWS IoT Core Policy som tillater maksimalt iot:Connect,
iot:Subscribe til control/#, iot:Subscribe til actuator/#, iot:Publish til startup/#, iot:Publish
til reading/#.

Aller best ville være om vi kunne kvitte oss med # og det bare er tillatt med
snappy/control/<device-class>, snappy/control/<device-id>, snappy/command/<device-id>,
snappy/startup/<device-class>/<device-id> og snappy/reading/<device-class>/<device-id>, men uklart
om dette lar seg gjøre i en stor flåte av devices: det blir ett sertifikat pr device, fordi det blir
én policy pr device og sertifikatet inneholder en policy.  Men det er en annen fordel med en mer
restriktiv policy: det blir mye mindre datatrafikk i en stor flåte av devices.

## Server

På server mottas meldingene og rutes etter hvert til en lambda.  Minimalt er det én for
snappy/startup/+/+ og én for snappy/reading/+/+, og for prototypen kjører vi dem når meldingen
kommer inn, men i et produksjonsmiljø kan det være attraktivt å bruke en basic-ingest funksjon og
deretter prosessere i batch for å holde kostnadene nede.  TBD.

En server / lambda skal ha lov å lytte på og publisere til alt, antakelig.

En lambda for snappy/startup/+/+ leser DEVICE og henter informasjon om devicets konfigurasjon.  Om
dette avviker fra standard (devicet er på) eller det som er rapportert i startup-meldingen sendes en
kontroll-melding til devicet for å konfigurere det.  I tillegg oppdaterer den et felt i HISTORY om
siste kontakt fra devicet (sekunder siden epoch) og skriver evt informasjon om plassering.

En lambda for snappy/reading/+/+ prosesserer måledataene.  Den oppdaterer HISTORY med siste kontakt
og legger til nye verdier i tabellene for sensorene, og forkaster eldste verdier.  Når HISTORY er
oppdatert beregnes ideal-verdiene for hver faktor som ble rapportert på stedet hvor denne sensoren
står.  Hvis ideal-verdien er ulik den leste verdien (kanskje innenfor en faktor som er spesifikk for
hver faktor?) og det er lenge nok siden sist en kommando ble sendt, sendes en action til aktivatoren
for faktoren på stedet, om denne er definert.

TODO: Aggregering over tid?

En ideal-funksjon spesifiseres som en streng, men denne har litt struktur: den er enten et tall (som
representerer ideal-verdien), navnet på en kjent funksjon (som da beregner ideal-verdien fra verdier
den selv har), eller navnet på en kjent funksjon med parametre på formatet
"navn/parameter/parameter".  (Litt uklart hvor komplekst dette bør gjøres.)  En typisk funksjon kan
være "arbeidsdag/22/20", som sier at temperaturen under arbeidsdagen (definert i funksjonen) skal
være 22 grader og under resten av tiden 20 grader.  Men minst like meningsfylt er om disse
parameterverdiene ligger i en tabell et sted og er satt av brukeren.
