<script>
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import { nextTick } from 'vue'


export default {
    props: [],
    emits: [],
    setup() {
    },
    data() {
        return {
            toasts: [],
            counter: 0,
        };
    },
    async created() {
        this.messageBus.on('show-toast', this.showToast);
    },
    async mounted() {
    },
    computed: {
    },
    watch: {
    },
    components: {},
    methods: {
        async showToast(toastMessage) {
            this.counter += 1;
            console.log(this.counter, toastMessage);
            this.toasts.push({title: "", message: toastMessage, id: this.counter});

            await nextTick();

            const toast = new bootstrap.Toast(document.getElementById('toast-' + this.counter));
            toast.show();
        },
    }

}
</script>


<template>
    <div id="toast-container" class="position-fixed bottom-0 end-0 p-3" style="z-index: 11;">
        <div v-for="toast in toasts" :id="'toast-' + toast.id" role="alert" aria-live="polite" aria-atomic="true"
            class="toast" data-bs-autohide="true">
            <div class="d-flex">
            <div class="toast-body">{{ toast.message }}
            </div>
            <button type="button" class="btn-close me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
            </div>
        </div>
    </div>

</template>

<style></style>