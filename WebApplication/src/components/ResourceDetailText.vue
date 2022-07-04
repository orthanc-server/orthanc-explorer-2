<script>
import CopyToClipboardButton from "./CopyToClipboardButton.vue";


export default {
    props: ["title", "value", "truncate", "copy"], // you can set copy=false if you don't wan't the copy button !
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
    },
    components: { CopyToClipboardButton }
}
</script>

<template>

    <li class="d-flex">
        <span class="details-label" :class="{'d-inline-block text-truncate': this.truncate}"><slot></slot>: </span>
        <span class="details">{{ value }}</span>
        <CopyToClipboardButton v-if="!this.copy && this.value && this.value.length > 0" :valueToCopy="this.value" />
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
