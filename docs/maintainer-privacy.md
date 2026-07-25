# Maintainer identity privacy

Public GameHQ materials identify the project, the `underfusion` GitHub account,
or repository roles. Windows binaries signed through the Foundation workflow
must display **SignPath Foundation** as publisher, not a maintainer's legal
name. This public boundary applies to source, documentation, manifests, SBOMs,
release evidence, packages, CI artifacts, logs, screenshots, and support files.

Legal name, private email, filled application fields, account identifiers,
session information, private correspondence, and SignPath responses must not be
committed or published. The SignPath application keeps identity fields as
maintainer-only placeholders; an agent must never invent those values, accept
terms, complete reCAPTCHA, or submit the form.

The maintainer should use a durable GameHQ-specific contact address privately
with SignPath and enable MFA on GitHub and SignPath. Future local commits use
GitHub's ID-based `users.noreply.github.com` address. Git history is not
rewritten merely to alter already-published metadata.

The maintainer has accepted the residual uncertainty about whether private
account identity can appear on a SignPath surface and waived a written privacy
inquiry. Recheck mandatory policy changes before submission, but do not block
solely on correspondence with SignPath. Keep any private response in encrypted
storage outside this repository.

The current SignPath privacy policy says account registration requires a name
and business email, administrators may provide a job title, and the service
processes IP/session/request-time data. It names Azure, SendGrid, and Okta as
subprocessors and currently states a six-month retention period for registration
data after account termination. Recheck the live policy immediately before
submission; this document is not a substitute for that review.
