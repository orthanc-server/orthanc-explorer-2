<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import { translateDicomTag } from "../locales/i18n"


export default {
    props: ["truncate", "copy", "tags", "tag", "showIfEmpty"], // you can set copy=false if you don't wan't the copy button !
    setup() {
        return {
        }
    },
    data() {
        return {
        }
    },
    mounted() {
    },
    methods: {
    },
    computed: {
        title() {
            return translateDicomTag(this.$i18n.t, this.$i18n.te, this.tag);
        },
        value() {
            if (this.hasValue) {
                return this.tags[this.tag];
            } else {
                return "";
            }
        },
        hasValue() {
            return this.tag in this.tags;
        },
        showValue() {
            return this.showIfEmpty || this.hasValue;
        }
    },
    components: { CopyToClipboardButton }
}
</script>

<template>

    <li v-if="showValue" class="d-flex">
        <span class="details-label" :class="{ 'd-inline-block text-truncate': this.truncate }" data-bs-toggle="tooltip"
            v-bind:title="title">{{ title }}: </span>
        <span class="details" data-bs-toggle="tooltip" v-bind:title="value">{{ value }}</span>
        <CopyToClipboardButton v-if="!this.copy && value && value.length > 0" :valueToCopy="value" />
    </li>

</template>

<style>
.details-label {
    font-weight: 700;
    max-width: 30%;
    overflow: hidden;
    text-overflow: ellipsis;
}

.details {
    margin-left: auto !important;
    font-weight: 500;
    max-width: 25vw;
    overflow: hidden;
    text-overflow: ellipsis;
}
</style>
