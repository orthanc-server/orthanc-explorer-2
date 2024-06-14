export default {
    filterLabel(str) {
        const regexLabel = new RegExp("^[0-9\-\_a-zA-Z]+$");
        if (str && str.length > 0 && !regexLabel.test(str)) {
            console.log("invalid label: ", str);
            const invalidLabelTips = document.querySelectorAll('.invalid-label-tips');
                invalidLabelTips.forEach(element => {
                    element.classList.remove('invalid-label-tips-hidden');
                })
            setTimeout(() => {
                const invalidLabelTips = document.querySelectorAll('.invalid-label-tips');
                invalidLabelTips.forEach(element => {
                    element.classList.add('invalid-label-tips-hidden');
                })
            }, 8000);
        } else {
            // console.log("valid '" + str + "'");
        }

        return str.replace(/[^0-9\-\_a-zA-Z]/gi, '');
    }
}