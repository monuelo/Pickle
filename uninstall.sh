confirm() {
    
    read -r -p "${1:--Are you sure you want to uninstall Pickle? [Yes/No]} " response
    case "$response" in
        [yY][eE][sS]|[yY])
            echo
            read -r -p "${1:-- Really? o.O [Yes/Nooooooooooooo]} " response
            case "$response" in
                [yY][eE][sS]|[yY])
                pwd=$(pwd)
                user=$(whoami)
                rm /home/$user/.local/bin/pickle
            ;;
        *)      
                show
            ;;
            esac
            ;;
        *)
            show
            ;;
    esac
}

show() {
    echo
    echo "- Thank you for choosing us, we will not disappoint you."
                
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
}
confirm