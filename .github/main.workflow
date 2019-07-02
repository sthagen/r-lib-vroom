workflow "Fix Documentation" {
  on = "push"
  resolves = [
    "Document Package"
  ]
}

action "Install Dependencies" {
  uses = "r-lib/ghactions/actions/install-deps@8347a162e105155a6ff7066f7c57461948551278"
}

action "Document Package" {
  uses = "r-lib/ghactions/actions/document@8347a162e105155a6ff7066f7c57461948551278"
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
