#!/bin/bash

# Usage: install_caf(version_number)
install_caf()
{
    ver=$1

    wget github.com/actor-framework/actor-framework/archive/${ver}.tar.gz -O caf-${ver}.tar.gz
    tar xzf caf-${ver}.tar.gz
    rm caf-${ver}.tar.gz

    cd actor-framework-${ver} && ./configure --prefix=/usr/local && make install
    cd ..
}

install_caf 0.14.4
