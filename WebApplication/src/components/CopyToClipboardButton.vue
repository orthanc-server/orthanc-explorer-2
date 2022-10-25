<script>
import clipboardHelpers from "../helpers/clipboard-helpers"

export default {
    props: ["valueToCopy"],
    setup() {
        return {
        }
    },
    data() {
        return {
            isSuccess: false,
            isError: false
        };
    },
    mounted() {
    },
    methods: {
        onSuccess() {
            console.log('copied to clipboard: ' + this.valueToCopy);
            this.isSuccess = true;
            this.isError = false;

            setTimeout(() => {
                this.isSuccess = false;
            }, 2500)
        },
        onFailure() {
            console.log('failed to copy to clipboard');
            this.isSuccess = false;
            this.isError = true;

            setTimeout(() => {
                this.isError = false;
            }, 2500)
        },
        click() {
            clipboardHelpers.copyToClipboard(this.valueToCopy,
                () => this.onSuccess(),
                () => this.onFailure()
            )
        }
    },
    computed: {
    },
    components: {}
}
</script>

<template>
    <button v-if="this.valueToCopy !== undefined" class="btn-clipboard"
        :class="{ 'success': isSuccess, 'error': isError }" type="button" data-bs-toggle="tooltip"
        title="Copy to clipboard" @click="click">
        <i v-if="this.isSuccess" class="bi-check"></i>
        <i v-if="this.isError" class="bi-bug"></i>
        <i v-if="!this.isSuccess && !this.isError" class="bi-clipboard"></i>
    </button>
</template>

<style>
.btn-clipboard {
    line-height: var(--bs-body-line-height);
    border: 0;
    background-color: transparent;
    border-radius: 0.2rem;
}

.success {
    color: green;
}

.error {
    color: red;
}
</style>
