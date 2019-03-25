workflow "Build, Check, Document and Deploy" {
  on = "push"
  resolves = [
    "Build Image",
    "Build Package",
    "Check Package"
  ]
}

action "Build Image" {
  uses = "actions/docker/cli@c08a5fc9e0286844156fefff2c141072048141f6"
  args = "build --tag=repo:latest ."
}

action "Check Package" {
  uses = "maxheld83/ghactions/Rscript-byod@master"
  needs = ["Build Image"]
  args = "-e 'devtools::install_dev_deps()' -e 'devtools::check(path = \".\", error_on = \"warning\")'"
}
