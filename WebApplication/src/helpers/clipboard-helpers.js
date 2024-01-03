export default {
    copyToClipboard(text, onSuccess, onFailure) {
        if (navigator.clipboard) {
            navigator.clipboard.writeText(text).then(
                () => {
                    if (onSuccess !== undefined) {
                        onSuccess();
                    } else {
                        console.log("copied to clipboard: ", text)
                    }
                },
                () => {
                    if (onFailure !== undefined) {
                        onFailure();
                    } else {
                        console.log("failed to copy to clipboard")
                    }
                }
            )
        } else { // this happens, e.g in unsecure contexts (https://discourse.orthanc-server.org/t/copy-icon-oe2-problem/3958/4)
            const tmp = document.createElement('TEXTAREA');
            const focus = document.activeElement;
        
            tmp.value = text;
        
            document.body.appendChild(tmp);
            tmp.select();
            document.execCommand('copy'); // note: this is deprecated but we use this code only as a fallback
            document.body.removeChild(tmp);
            focus.focus();
            
            if (onSuccess !== undefined) {
                onSuccess();
            } else {
                console.log("hopefully copied to clipboard: ", text)
            }
        } 
    },
}
