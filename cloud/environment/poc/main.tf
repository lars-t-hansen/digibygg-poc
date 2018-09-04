provider "aws" {
  region = "eu-central-1"
}

variable "account_id" {
  description = "Account id for the owner of the image"
}

data "aws_ami" "influx_and_grafana_image" {
  most_recent      = true

  filter {
    name   = "name"
    values = ["influx-graphana*"]
  }

  owners = ["${var.account_id}"]
}

module "infrastructure" {
  source = "../../infrastructure/aws"
  environment = "poc"
  project     = "sense"

  influxdb_ami = "${data.aws_ami.influx_and_grafana_image.id}"
}