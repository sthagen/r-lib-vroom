workflow "Fix Documentation" {
  on = "push"
  resolves = [
    "Document Package"
  ]
}

action "Install Dependencies" {
  uses = "r-lib/ghactions/actions/install-deps@29c5ac0349c783689bf1f0f0a5fde40baa6002fb"
}

action "Document Package" {
  uses = "r-lib/ghactions/actions/document@29c5ac0349c783689bf1f0f0a5fde40baa6002fb"
  needs = [
    "Install Dependencies"
  ]
  args = [
    "--after-code=commit"
  ]
  secrets = [
    "GITHUB_TOKEN"
  ]
}
