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

action "Build Package" {
  needs = "Build Image"
  uses = "maxheld83/ghactions/Rscript-byod@master"
  args = "-e 'devtools::build(path = \".\")'"
}

action "Check Package" {
  uses = "maxheld83/ghactions/Rscript-byod@master"
  needs = ["Build Package"]
  args = "-e 'devtools::check_built(path = \".\", error_on = \"warning\")'"
}
