-*- fill-column: 100 -*-

# SnappySense MQTT-protokoll

## Meldingsformat

Ved oppstart sender devicet en melding `snappy/startup/<device-class>/<device-id>` med en JSON-pakke:

```
  { time: <integer, seconds since epoch>,
    reading_interval: <integer, seconds between readings> }
```

hvor `reading_interval` bare blir sendt med om devicet er en sensor.  Ved oppstart er devicet normalt
`On`, dvs, det vil sende observasjoner om det ikke får beskjed om annet.

Ved observasjon sender devicet en melding med topic `snappy/reading/<device-class>/<device-id>`:

```
  { time: <integer, seconds since epoch>,
    ... }
```

I denne pakken kan devicet sende med felter som representerer siste avlesing av de sensorer som
måtte finnes på devicet, bl.a. disse:

```
  temperature: <integer, degrees celsius>,
  humidity: <integer, relative in interval 0..100>,
```

AWS IoT kan sende kontrollmeldinger til devicet.  Disse sendes til topic `snappy/control/<device-id>`,
`snappy/control/<device-class>` og `snappy/control/all`, som devicet kan abonnere på.  Device-class
indikerer typen device.

Kontrolldataene kan være blant disse:

```
  enable: <number, 0 | 1>,
  reading_interval: <number, positive number of seconds between readings>,
```

hvor enable kontrollerer om devicet utfører og rapporterer målinger, og reading_interval
kontrollerer hvor ofte målingene skjer.

AWS IoT kan også sende kommando-meldinger til devicet.  Disse sendes til `snappy/command/<device-id>`
(og ikke til verken device-class eller til `all`, det gir antakelig ikke mening) og kan inneholde
meldinger som disse:

```
  { actuator: <string>, reading: <value>, ideal: <value> }
```

Meningen her er at den navngitte actuator, om den finnes, skal slå inn på en sånn måte at
forskjellen mellom reading og ideal blir mindre.  (Her er det rom for å tenke annerledes / mer.)

Hver klient har en unik device ID (som bare har betydning for applikasjonen) og tilhører en device
klasse (ditto).

## Policy

### Klient

Hver klient skal ha et sertifikat med en AWS IoT Core Policy som tillater maksimalt:

* `iot:Connect`
* `iot:Subscribe` til `snappy/control/+`
* `iot:Subscribe` til `snappy/command/+`
* `iot:Publish` til `snappy/startup/+/+`
* `iot:Publish` til `snappy/reading/+/+`.

TODO: Aller best ville være om vi kunne kvitte oss med `+` og pub/sub bare kan være med devicets egen klasse
og ID, men det er foreløpig uklart om dette lar seg gjøre i en stor flåte av devices.  Men i tillegg til
sikkerhet er det en annen fordel med en mer restriktiv policy: det blir mye mindre datatrafikk i en stor
flåte av devices.

### Server

En server / lambda skal ha lov å lytte på og publisere til alt, antakelig.
