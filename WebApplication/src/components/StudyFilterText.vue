<script>
import { useIsMobileAny, useIsMobileWide } from "../helpers/media-helpers";

export default {
    props: {
        columnTag: String,
        placeholder: String,
        filterClass: String,
        // inputClass: { default: "form-control study-list-filter" },
        // values from StudyList
        // textValue: String,        // filterGenericTags[columnTag]
    },
    data() {
        return {
            textValue: null,
            isMobileAny: useIsMobileAny(),
            isMobileWide: useIsMobileWide(),
        }
    },
    created() {
        this.messageBus.on('set-text-filter', this.setTextValue);
        this.messageBus.on('clear-text-filter', this.clearTextValue);
    },
    computed: {
        inputClass() {
            if (this.isMobileAny && !this.isMobileWide) {
                return 'form-control study-list-filter w-100 py-1';
            } else {
                return 'form-control study-list-filter'
            }
        }
    },
    methods: {
        setTextValue(message) {
            if (message.textTag == this.columnTag) {
                this.textValue = message.textValue;
            }
        },
        clearTextValue(message) {
            if (message.textTag == this.columnTag) {
                this.textValue = null;
            }
        },
        textChanged(event) {
            this.textValue = event.target.value;

            // console.log("datePicked: ", event);
            this.messageBus.emit("text-filter-updated", {textTag: this.columnTag, textValue: this.textValue});
        },

    }
}
</script>

<template>
    <input type="text" :class="[inputClass, filterClass]"
        :value="textValue" @input="textChanged($event)"
        :placeholder="placeholder" :id="'filter-' + columnTag" />
</template>