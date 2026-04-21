<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import { mapState, mapGetters } from "vuex"


export default {
    props: ["title", "value", "showIfEmpty", "isError"],
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
            return (this.showIfEmpty != false) && this.value;
        }
    },
}
</script>

<template>
    <div v-if="showValue" style="display: flex; justify-content: space-between;" :class="{'text-danger': isError}">
        <span class="text-start"><strong>{{ title }}:</strong></span>
        <span class="text-end">{{ value }}</span>
    </div>
</template>

<style>
</style>
