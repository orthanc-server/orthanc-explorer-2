export default {
    copyToClipboard(text, onSuccess, onFailure) {
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
    },
}
