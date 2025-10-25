# ClockOut

ClockOut is a terminal based companion for tracking the rest of your workday.
It renders a large countdown timer (or a set of discrete blocks) that lets you
see how long remains in your scheduled shift, lunch, and breaks.  When paired
with the optional time bank, ClockOut will also persist any overtime you accrue
so you can keep an eye on your flex time balance from one session to the next.

## Features

* **Flexible start times** – begin the countdown from `HH:MM`, `HHhMM`, or
  `HH:MMam/pm` formatted timestamps.
* **Adjustable lunch and break durations** – override the default 45 minute
  lunch and add ad-hoc breaks during the day.
* **Visual display modes** – choose between a large digital timer or a
  minimalist discrete block visualization.
* **Persistent time bank** – optionally store surplus time in a JSON backed
  ledger in your home directory.

## Building

ClockOut is a single C program that depends on `ncursesw` and `json-c`.  With
those libraries installed, build the executable using `make`:

```bash
make
```

The resulting binary (`clockout`) will be written to the repository root.

## Usage

Run the compiled program from your terminal.  By default it assumes an eight
hour shift (`480` minutes) that started when the program launched and includes a
45 minute lunch.  You can override these defaults and enable optional features
via command-line arguments:

```bash
./clockout [START_TIME] [LUNCH_MINUTES] [WORK_HOURS] [options]
```

### Common arguments

* `START_TIME` – Provide the shift start as `HH:MM`, `HHhMM`, or include an `am`
  / `pm` suffix (e.g. `8:30am`, `14h15`).
* `LUNCH_MINUTES` – Override the default lunch length by passing a number with
  an `m` suffix, e.g. `30m` or `60m`.
* `WORK_HOURS` – Supply the total work duration in hours using an `h` suffix,
  e.g. `7h` for a seven hour shift.

### Options

* `--time-bank <hours>` / `-tb <hours>` – Enable the time bank and seed it with
  an initial balance.  The ledger is stored at
  `~/.work_timer_timebank.json`.
* `--discrete` / `-d` – Start in discrete block rendering mode instead of the
  large digital timer.

### In-app shortcuts

* Press `b` to add additional break minutes during a session.
* Press `d` to toggle discrete mode while the timer is running.
* Press `q` twice to quit the program.

### Examples

```bash
# Start from an 8:00 AM shift with a 30 minute lunch
./clockout 8:00am 30m

# Start at 9:15, work a 7 hour shift, enable the time bank with 1 hour
./clockout 9h15 45m 7h --time-bank 1

# Launch directly into discrete visualization mode
./clockout --discrete
```

## Cleaning up

Remove the compiled binary and any intermediate files with:

```bash
make clean
```

## Packaging

Dockerfiles are provided to build native packages for several Linux
distributions. Each image compiles ClockOut, assembles the appropriate package
format, and writes the result to `/dist` inside the container. Invoke them from
the repository root, overriding the `VERSION` build argument if needed:

```bash
# Build a Debian package (clockout_VERSION_amd64.deb)
docker build -f docker/debian/Dockerfile -t clockout-debian .

# Build an RPM for Red Hat compatible systems
docker build -f docker/redhat/Dockerfile -t clockout-redhat .
```

Available targets:

| Distro    | Dockerfile path                | Package output                                |
|-----------|--------------------------------|-----------------------------------------------|
| Arch      | `docker/arch/Dockerfile`       | `clockout-<version>-1-x86_64.pkg.tar.zst`      |
| Debian    | `docker/debian/Dockerfile`     | `clockout_<version>_amd64.deb`                 |
| Alpine    | `docker/alpine/Dockerfile`     | `clockout-<version>-r0.apk`                    |
| Red Hat   | `docker/redhat/Dockerfile`     | `clockout-<version>-1.el9.x86_64.rpm`          |
| openSUSE  | `docker/suse/Dockerfile`       | `clockout-<version>-1.x86_64.rpm`              |
| Slackware | `docker/slackware/Dockerfile`  | `clockout-<version>-x86_64-1_clockout.txz`     |
| Gentoo    | `docker/gentoo/Dockerfile`     | `clockout-<version>.tbz2`                      |
| Ubuntu    | `docker/ubuntu/Dockerfile`     | `clockout_<version>_amd64.deb`                 |

Copy the artifact out of the container with `docker create` / `docker cp` or by
using a multi-stage build tailored to your workflow.
