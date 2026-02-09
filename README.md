
# JadDriver

Small Windows kernel-mode driver I wrote as a quick exercise to refresh WDM basics and IOCTL handling.

Nothing fancy — just a clean, minimal driver that creates a device, exposes a few buffered IOCTLs, and unloads properly.

# What it does

Creates a device and DOS symbolic link

Handles CREATE, CLOSE, and DEVICE_CONTROL IRPs

Uses buffered IOCTLs (METHOD_BUFFERED)

Simple echo IOCTL for testing

Two stub IOCTLs that return mock keyboard/mouse info

# What it does not do

No process or memory manipulation

No hooks, callbacks, or filters

No interaction with real input device stacks

No anti-cheat or evasion logic

Keyboard/mouse responses are mock data only — this is just an IOCTL demo.

# Why this exists

I built this as a quick standalone kernel project to:

stay sharp on Windows driver structure

practice safe IOCTL patterns

have a clean, public example that isn’t tied to any exploit or cheat work

There’s no user-mode client included — the interface is simple and the focus here is the kernel side.

# Build

Windows Driver Kit (WDK)

Visual Studio

x64 target
