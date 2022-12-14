-*- fill-column: 100 -*-

# Misc design notes

(Much missing here, needs to be moved)

## Aggregating historical data somehow

- this begs the purpose of gathering the data in the first place
- we want to record "conditions" and monitor them over time and alternatively also provide an
  indicator when "conditions" are bad
- so meaningfully we might consider weekday as the primary key for a datum, as we care more about
  comparing mondays to mondays than january 1 to january 1
- so there may be a table that is indexed by (location, day-of-the-week) and for each entry there is
  a table going back some number of weeks (TBD, but maybe 100?), and in each table element there is
  an object (factor, [{time, reading, ...}, ...])  where the exact fields of the inner structures
  are tbd.  This should be sorted with most recent timestamp first in the inner list.
- there could be a lot of data here if we have one reading per factor per hour, this should be a
  consideration.  In principle, the table could be indexed by a triple, (location, day-of-the-week,
  hour-of-the-day)
- this all makes it easy to report per-location and per-hour and/or per-day-of-the-week.

## API

The sensors, actuators, and backend communicate by MQTT, see mqtt-protocol.md in this directory.

The frontend and the server communicate by REST.  But what is the API?

- obvious first report is to get a plot (say, or even a table) of the reports for a factor for
  a location over time
- so there is some kind of selector for location, a selector for factor, and a selector for a
  date range (could even be "last week", "last month", "last year") and maybe for a time of
  day, this turns into a GET or POST and back comes a page