# ir-turret-tuning — agent skill

An [agent skill](https://docs.claude.com/en/docs/agents-and-tools/agent-skills) that reproduces the
debugging session behind this repo: drives a CrunchLabs IR Turret over USB serial, rules out power,
separates rotation problems from release problems, and ladders `rollPrecision` / `rollStep` to a
confirmed setting.

## Install

**Claude Code** — copy into your skills directory:

```bash
mkdir -p ~/.claude/skills
cp -r skills/ir-turret-tuning ~/.claude/skills/
```

Then ask: *"my Hack Pack turret only fires 2 of 6 darts"* — or invoke it directly with
`/ir-turret-tuning`.

**Other agents** — `SKILL.md` is plain Markdown with YAML frontmatter; paste it as a system prompt
or tool description.

## What it does

1. Opens the serial link (with the `exec` ordering that actually works on macOS)
2. Rules out brownout using uptime
3. Separates under-rotation from peg/elastic failures
4. Ladders the base timing, reading *which* shots miss
5. Adds a per-shot ramp
6. Requires two consistent magazines before baking anything in

It also carries the statistical discipline that made the session work: one magazine is n=1,
sub-10 ms steps are below the noise floor, and the board cannot see the barrel — only the user can.
