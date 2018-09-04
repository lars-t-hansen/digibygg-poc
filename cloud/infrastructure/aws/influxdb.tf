## Variables ##
variable "influxdb_ami" {
  description = "AMI id for influxdb instance"
}

variable "av_zone" {
  description = "The availability zone to put the instance in"
  default = "eu-central-1a"
}

variable "influxdb_instance_type" {
  description = "Instance type for influxdb"
  default = "t3.micro"
}

variable "public" {
  description = "True if public. Give the instance a public ID"
  default = true
}

## Resources ##
resource "aws_instance" "influxdb" {
  ami = "${var.influxdb_ami}"
  availability_zone = "${var.av_zone}"
  instance_type = "${var.influxdb_instance_type}"
  vpc_security_group_ids = [
    "${aws_security_group.allow_ssh.id}", 
    "${aws_security_group.allow_grafana.id}", 
    "${aws_security_group.allow_influxdb.id}"
  ]
  subnet_id = "${aws_subnet.public_a.id}"
  associate_public_ip_address = "${var.public}"

  tags {
    Name = "${var.environment}.influxdb"
    Project = "${var.project}"
  }
}

resource "aws_security_group" "allow_http" {
  name = "http-${var.project}-${var.environment}"
  vpc_id = "${aws_vpc.main.id}"
  description = "Allow HTTP connections"

  ingress {
    from_port = 80
    to_port = 80
    protocol = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port = 0
    to_port = 0
    protocol = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags {
    Name = "http.${var.project}.${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_security_group" "allow_ssh" {
  name = "ssh-${var.project}-${var.environment}"
  vpc_id = "${aws_vpc.main.id}"
  description = "Allow SSH connections"

  ingress {
    from_port = 22
    to_port = 22
    protocol = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port = 0
    to_port = 0
    protocol = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags {
    Name = "ssh.${var.project}.${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_security_group" "allow_grafana" {
  name = "grafana-${var.project}-${var.environment}"
  vpc_id = "${aws_vpc.main.id}"
  description = "Allow grafana"

  ingress {
    from_port = 3000
    to_port = 3000
    protocol = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port = 0
    to_port = 0
    protocol = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags {
    Name = "grafana.${var.project}.${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_security_group" "allow_influxdb" {
  name = "influxdb-${var.project}-${var.environment}"
  vpc_id = "${aws_vpc.main.id}"
  description = "Allow connections to InlufxDB"

  ingress {
    from_port = 8086
    to_port = 8086
    protocol = "tcp"
    self = true
  }

  egress {
    from_port = 0
    to_port = 0
    protocol = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags {
    Name = "influxdb.${var.project}.${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}