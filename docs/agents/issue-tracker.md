# Issue tracker: Local Markdown

Issues and specifications for this repository live as Markdown files under `.scratch/`.

## Conventions

- Use one directory per effort: `.scratch/<feature-slug>/`.
- Store its specification at `.scratch/<feature-slug>/spec.md`.
- Store implementation tickets individually at
  `.scratch/<feature-slug>/issues/<NN>-<slug>.md`.
- Number tickets from `01`; do not combine all tickets into one file.
- Record workflow state with a `Status:` line near the top when a skill
  requires it.
- Append discussion and history under a `## Comments` heading.

## Publishing

When a skill says to publish to the issue tracker, create the appropriate
Markdown file under `.scratch/<feature-slug>/`, creating its directories as
needed.

When a skill says to fetch a ticket, read the referenced local file. The user
will normally provide its path or ticket number.

## Wayfinding operations

A wayfinding effort uses one map and one child file per ticket.

- Map: `.scratch/<effort>/map.md`
- Child ticket: `.scratch/<effort>/issues/<NN>-<slug>.md`
- Ticket types: `research`, `prototype`, `grilling`, or `task`
- Ticket states: `claimed` or `resolved`
- Dependencies: `Blocked by: NN, NN`

To claim work, set `Status: claimed` before starting. To resolve it, append
the result under `## Answer`, set `Status: resolved`, and add a concise
decision pointer to the map.
