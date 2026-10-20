<script>
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import { nextTick } from 'vue'

document.toastCounter = 0;

export default {
    props: [],
    emits: [],
    setup() {
    },
    data() {
        return {
            toasts: []
        };
    },
    async created() {
    },
    async mounted() {
        this.messageBus.on('show-toast', this.showToast);
    },
    async beforeUnmount() {
        this.messageBus.off('show-toast', this.showToast);
    },
    computed: {
    },
    watch: {
    },
    components: {},
    methods: {
        async showToast(toastMessage) {
            document.toastCounter += 1;
            
            if (typeof(toastMessage) === 'string') {
                toastMessage = { 
                    'message': toastMessage,
                    'type': 'success'    // success, warning, info, danger/error
                }
            }
            toastMessage['id'] = document.toastCounter;

            // console.log(this.counter, toastMessage);
            this.toasts.push(toastMessage);

            await nextTick();
            await this.$nextTick();

            const toast = new bootstrap.Toast(document.getElementById('toast-' + document.toastCounter));
            toast.show();
        },
        classes(toast) {
            if (toast['type'] == 'success') {
                return "bg-success text-white";
            } else if (toast['type'] == 'info') {
                return "bg-info text-dark";
            } else if (toast['type'] == 'danger' || toast['type'] == 'error') {
                return "bg-danger text-white";
            } else {
                return "bg-warning text-dark";
            }
        }
    }

}
</script>


<template>
    <div id="toast-container" class="position-fixed bottom-0 end-0 p-3" style="z-index: 11;">
        <div v-for="toast in toasts" :id="'toast-' + toast.id" role="alert" aria-live="polite" aria-atomic="true"
            class="toast p-2" :class="classes(toast)" :data-bs-autohide="true">
            <div class="d-flex">
                <div class="toast-body">{{ toast.message }}
                </div>
                <button type="button" class="btn-close me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
            </div>
        </div>
    </div>

</template>

<style></style>