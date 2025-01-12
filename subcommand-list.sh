#!/bin/sh -e

cat << EOM > subcommands.md
# Lpjs subcommand list

Each subcommand below is documented by a man page.  To view the documentation,
install lpjs using your chosen package manager and run \`man subcommand\`
(e.g. \`man lpjs-nodes\`).

| Subcommand | Purpose |
|------------|---------|
EOM

# Man page
cat << EOM > Man/lpjs.1
.TH lpjs 1

.SH NAME
lpjs - Lightweight, Portable Job Scheduler

.SH "DESCRIPTION"
.B LPJS
is a batch system, i.e. a job scheduler and resource manager, for running
programs in the background when resources available.

.SH subcommand
.nf
.na
EOM

auto-man2man Man/lpjs-* >> Man/lpjs.1

cat << EOM >> Man/lpjs.1
.ad
.fi

.SH "SEE ALSO"
munge(1), munge(3), munge(7)

.SH AUTHOR
.nf
.na
J Bacon
EOM

# Debug
# man Man/lpjs.1

# For github
auto-man2man Man/* | awk -F - '$1 !~ "lpjs" { printf("| %s | %s |\n", $1, $2); }' \
    >> subcommands.md

# Debug
# grip --export subcommands.md
# firefox ./subcommands.html
