# Security policy

This client connects to systems that monitor and control industrial and
power-system equipment. A defect here can have consequences beyond the
workstation it runs on, so please report suspected vulnerabilities privately
rather than in public.

## Reporting a vulnerability

**Email [alexsmn@gmail.com](mailto:alexsmn@gmail.com)** with "security" in
the subject line.

Please do **not** open a public issue, a pull request or a discussion for
anything you believe is exploitable. A public report is readable by everyone
running this software, including people running it on live plant, before there
is anything for them to upgrade to.

Useful things to include, as far as you have them:

- What an attacker can do, and what they need in order to do it — network
  position, credentials, a particular server configuration
- The steps to reproduce it, and the client version (Help → About, or the
  right-hand end of the status strip)
- Your OS, and which data service backend you were connected to (Scada,
  OPC UA or Vidicon)
- Whether the issue is in this client, in the server it talks to, or in the
  protocol between them, if you can tell

Findings in the protocol stacks this client speaks — OPC UA, and the
Telecontrol gRPC session protocol — are in scope even when the defect turns out
to be on the server side of the conversation.

## What happens next

Reports reach the maintainer directly and are acknowledged by reply. We will
tell you what we find, whether we consider it a vulnerability, and when a fix
ships. If you would like credit in the release notes, say so;
if you would prefer not to be named, say that instead.

We have no bug bounty. This is a request for responsible disclosure, not a
paid programme.

## A note on how fixes reach this repository

This repository is a **generated export** of a private development tree — see
[CONTRIBUTING.md](CONTRIBUTING.md). Fixes are made upstream and arrive here
through the next export, so a security fix will not appear as a pull request
against this repository and cannot be merged as one. That is also why a patch
is more useful attached to your report than opened as a PR.

## Scope

This policy covers the Telecontrol SCADA client in this repository. The
libraries it consumes are published separately and carry their own history;
report an issue you have traced into one of them here anyway, and say which —
it reaches the same maintainers.

Deployment questions — how a particular installation is configured, exposed or
firewalled — are support questions rather than vulnerability reports, and the
same address handles them.
