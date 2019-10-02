#!/bin/bash

echo '- Welcome to the Pickle installation wizard'
echo
read -r -p "${1:-- You want to install g++ and make? [Yes/No]} " response
    case "$response" in
        [yY][eE][sS]|[yY])
        sudo apt-get install g++ make
    ;;
    esac
echo


installed=true
echo "- Now, let's configure the execution file..."
{
    pwd=$(pwd)
    user=$(whoami)
    
    cd $pwd/src/
    make -s > /dev/null
    cd ..

    echo $pwd

    mkdir bin
    mv $pwd/src/pickle /$pwd/bin
    cp $pwd/bin/pickle /home/$user/.local/bin > /dev/null
} || {
    echo
    echo "INSTALLATION FAILED: Something wrong has occurred!"
    installed=false
}

export PATH=$PATH:/home/$user/.local/bin    
source /etc/environment
echo
if $installed ; then
    echo "- Thank's for install Pickle, run 'pickle' on terminal to start"
echo "
            ██████╗ ██╗ ██████╗██╗  ██╗██╗     ███████╗
            ██╔══██╗██║██╔════╝██║ ██╔╝██║     ██╔════╝
            ██████╔╝██║██║     █████╔╝ ██║     █████╗  
            ██╔═══╝ ██║██║     ██╔═██╗ ██║     ██╔══╝  
            ██║     ██║╚██████╗██║  ██╗███████╗███████╗
            ╚═╝     ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝" 
            echo "
             The Best Text Editor Text User Interface"
            echo
            echo
fi

