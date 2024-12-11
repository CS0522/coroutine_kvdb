#!/usr/bin/env bash

# Run as root

function install_jdk_mvn_ubuntu()
{
    # ----- install jdk 11 and maven3 ----- #
    apt-get install -y maven
    # get java install path
    local java_path=`ls -lrt /etc/alternatives/java | awk -F'> ' '{print $2}' | awk -F'/bin' '{print $1}'`
    echo "java install path: ${java_path}"
    # configure JAVA_HOME
    echo "export JAVA_HOME=${java_path}" >> /etc/profile
    echo 'export CLASSPATH=$JAVA_HOME/lib:$CLASSPATH' >> /etc/profile
    echo 'export PATH=$JAVA_HOME/bin:$PATH' >> /etc/profile
    source /etc/profile

    # get maven install path
    local maven_path=`ls -lrt /etc/alternatives/mvn | awk -F'> ' '{print $2}' | awk -F'/bin' '{print $1}'`
    echo "maven install path: ${maven_path}"
    # configure MAVEN_HOME
    echo "export MAVEN_HOME=${maven_path}" >> /etc/profile
    echo 'export PATH=$PATH:$MAVEN_HOME/bin' >> /etc/profile
    source /etc/profile
}

function install_jdk_mvn_centos()
{
    # ----- install jdk 1.8 and maven3 ----- #
    yum install -y maven
    # get java install path
    local java_path=`ls -lrt /etc/alternatives/java | awk -F'> ' '{print $2}' | awk -F'/jre' '{print $1}'`
    echo "java install path: ${java_path}"
    # configure JAVA_HOME
    echo "export JAVA_HOME=${java_path}" >> /etc/profile
    echo 'export JRE_HOME=$JAVA_HOME/jre' >> /etc/profile
    echo 'export CLASSPATH=$JAVA_HOME/lib:$CLASSPATH' >> /etc/profile
    echo 'export PATH=$JAVA_HOME/bin:$JRE_HOME/bin:$PATH' >> /etc/profile
    source /etc/profile

    # get maven install path
    local maven_path=`ls -lrt /etc/alternatives/mvn | awk -F'> ' '{print $2}' | awk -F'/bin' '{print $1}'`
    echo "maven install path: ${maven_path}"
    # configure MAVEN_HOME
    echo "export MAVEN_HOME=${maven_path}" >> /etc/profile
    echo 'export PATH=$PATH:$MAVEN_HOME/bin' >> /etc/profile
    source /etc/profile
}

function install_fn()
{
    # Ubuntu or CentOS
    local sys=`uname -a | grep Ubuntu`
    if [ -n "${sys}" ]; then
        install_jdk_mvn_ubuntu
    else
        install_jdk_mvn_centos
    fi
}

install_fn
# source /etc/profile after changing to normal user