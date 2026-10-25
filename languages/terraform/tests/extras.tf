locals {
  nothing = nul

  notes = <<DESCRIPTION
A block comment tag distinct from EOF.
DESCRIPTION

  readme = <<-NOTES
    An indented heredoc with its own tag, not the fixed EOF/DESCRIPTION
    pair the reference grammar hardcodes.
NOTES

  usage = <<-EOT
    A <<- heredoc whose closing tag is indented with the block.
    EOT

  generated_at = timestamp()

  trailing = [1, 2, 3,]

  negated_ref  = -local.trailing[0]
  not_disabled = !local.nothing
}

data "example_thing" "sample" {
  provider = aws.in

  metadata = {
    in       = "reserved words read as names where the state allows it"
    provider = "same for provider"
  }
}

resource "example_resource" "sample" {
  endpoint "primary" "secondary" {
    host = "a"
    port = 80
  }
}
