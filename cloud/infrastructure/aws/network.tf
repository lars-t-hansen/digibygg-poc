## Variables ##

## Resources ##
resource "aws_vpc" "main" {
  cidr_block = "10.0.0.0/16"

  tags {
    Name        = "vpc-${var.project}-${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

## Internet gateway ##
resource "aws_internet_gateway" "internet_gateway" {
  vpc_id = "${aws_vpc.main.id}"

  tags {
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

## Nat gateway ##
resource "aws_nat_gateway" "nat_gateway" {
    allocation_id = "${aws_eip.nat_gateway_eip.id}"
    subnet_id = "${aws_subnet.public_a.id}"
    depends_on = ["aws_internet_gateway.internet_gateway"]
}

resource "aws_eip" "nat_gateway_eip" {
    vpc = true
}

## Public route table ##
resource "aws_route_table" "public" {
  vpc_id = "${aws_vpc.main.id}"

  tags {
    Name        = "public-route-table-${var.project}-${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

## Route for public route table ##
resource "aws_route" "public" {
  route_table_id          = "${aws_route_table.public.id}"
  gateway_id              = "${aws_internet_gateway.internet_gateway.id}"
  destination_cidr_block  = "0.0.0.0/0"
}

## Private route table ##
resource "aws_route_table" "private" {
  vpc_id = "${aws_vpc.main.id}"

  tags {
    Name        = "private-route-table-${var.project}-${var.environment}"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

## Route for private route table ##
resource "aws_route" "private" {
  route_table_id = "${aws_route_table.private.id}"

  destination_cidr_block = "0.0.0.0/0"
  nat_gateway_id = "${aws_nat_gateway.nat_gateway.id}"
}

## Route assouciation table. Connects subnets with the route table ##
resource "aws_route_table_association" "public_a" {
  subnet_id = "${aws_subnet.public_a.id}"
  route_table_id = "${aws_route_table.public.id}"
}

resource "aws_route_table_association" "public_b" {
  subnet_id = "${aws_subnet.public_b.id}"
  route_table_id = "${aws_route_table.public.id}"
}

resource "aws_route_table_association" "public_b_assign_ip" {
  subnet_id = "${aws_subnet.public_b_assign_ip.id}"
  route_table_id = "${aws_route_table.public.id}"
}

resource "aws_route_table_association" "private_a" {
  subnet_id = "${aws_subnet.private_a.id}"
  route_table_id = "${aws_route_table.private.id}"
}

resource "aws_route_table_association" "private_b" {
  subnet_id = "${aws_subnet.private_b.id}"
  route_table_id = "${aws_route_table.private.id}"
}

## Subnets ##
resource "aws_subnet" "private_a" {
  vpc_id = "${aws_vpc.main.id}"
  availability_zone = "eu-central-1a"
  cidr_block = "10.0.1.0/24"

  tags {
    Name        = "${var.project}.${var.environment}.private.subnet.a"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_subnet" "private_b" {
  vpc_id = "${aws_vpc.main.id}"
  availability_zone = "eu-central-1b"
  cidr_block = "10.0.2.0/24"

  tags {
    Name        = "${var.project}.${var.environment}.private.subnet.b"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_subnet" "public_a" {
  vpc_id = "${aws_vpc.main.id}"
  availability_zone = "eu-central-1a"
  cidr_block = "10.0.4.0/24"

  tags {
    Name        = "${var.project}.${var.environment}.public.subnet.b"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_subnet" "public_b" {
  vpc_id = "${aws_vpc.main.id}"
  availability_zone = "eu-central-1b"
  cidr_block = "10.0.5.0/24"

  tags {
    Name        = "${var.project}.${var.environment}.public.subnet.b"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}

resource "aws_subnet" "public_b_assign_ip" {
  vpc_id = "${aws_vpc.main.id}"
  availability_zone = "eu-central-1b"
  cidr_block = "10.0.10.0/24"
  map_public_ip_on_launch = true

  tags {
    Name        = "${var.project}.${var.environment}.public_b.assign.public.ip"
    Environment = "${var.environment}"
    Project     = "${var.project}"
  }
}
