# Path Variable Handlers [![Build Status](https://travis-ci.org/sidoh/path_variable_handlers.svg?branch=master)](https://travis-ci.org/sidoh/path_variable_handlers)

This is a library for handling paths. Many RESTful APIs contain resources that have variables in their paths (e.g., `/things/:thing_id`).  This library exposes a way to process such resource paths in a low-effort way.

While this is mostly useful for REST APIs, it could also be used, for example, to process MQTT topics containing wildcards.

There is nothing platform-specific about this library.

## Examples

Examples are found in the `./examples` directory.  This is the easiest way to get started.

## Development

Run tests with:

```
platformio test
```

Build examples with, for instance:

```
platformio ci --board=d1_mini --lib=. examples/simple
```

#### New Release

1. Update version in `library.properties` and `library.json`.
1. Create a new tag with the corresponding version and push.