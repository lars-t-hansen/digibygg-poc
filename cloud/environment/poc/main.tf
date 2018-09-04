provider "aws" {
  region = "eu-central-1"
}
module "infrastructure" {
  source = "../../infrastructure/aws"
  environment = "poc"
  project     = "sense"

  influxdb_ami = "ami-0c39bb7d1fe8951e7"
}