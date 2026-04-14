<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import { mapState, mapGetters } from "vuex"


export default {
    props: ["title", "value", "showIfEmpty"],
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
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        showValue() {
            return (this.showIfEmpty != false) || this.hasValue;
        }
    },
}
</script>

<template>
    <div v-if="showValue" style="display: flex; justify-content: space-between;">
        <span><strong>{{ title }}:</strong></span>
        <span>{{ value }}</span>
    </div>
</template>

<style>
</style>
