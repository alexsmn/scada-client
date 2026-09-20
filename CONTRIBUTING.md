# Contributing

Thanks for looking. Please read the first section before opening a pull
request — this repository cannot accept one, and that is a property of how it
is published rather than a judgement about the change.

## This repository is generated

`scada-client` is an **export**. Development happens in a private monorepo
alongside the products this client is built with, and the public repository is
regenerated from it by path filtering and pushed. History is appended and never
rewritten.

The consequence for pull requests is mechanical. A merge commit here moves the
public branch to a commit the monorepo does not have, so the next publish is no
longer a fast-forward — and the exporter refuses a non-fast-forward rather than
force-pushing over it. A merged PR therefore does not get overwritten; it stops
publication until someone reconciles the two histories by hand. That is why the
answer is "please don't", not "we'll get to it".

## What is welcome

**Open an issue.** Bug reports, questions about behaviour, and errors in the
documentation are all useful, and an issue costs you less than a patch that
cannot be merged.

Useful things to include:

- What you did, what happened, and what you expected instead
- The client version (Help → About, or the status strip's right-hand end)
- Your OS and version
- Which data service backend you were connected to — Scada, OPC UA or Vidicon

**Patches are still welcome as patches.** If you have a fix, attach a diff or
describe the change in an issue. It can be applied upstream with attribution
and will reach this repository through the next export.

## Note that this repository does not build on its own

Three of the six products the client consumes are not published yet, so a clone
is sources to read rather than a build to run. The README's "Trying it" section
says which, and the live demo is the quickest way to see the software working.
That also means CI here is static analysis only — a green check is cppcheck, not
a build and not a test run.
