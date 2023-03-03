<script>
import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: [],
    async created() {
        const params = new URLSearchParams(window.location.search);
        const response = await api.parseToken('token', params.get('token'));
        this.tokenChecked = true;

        if ('ErrorCode' in response) {
            this.errorCode = response['ErrorCode'];
        } else if ('RedirectUrl' in response) {
            window.location = response['RedirectUrl'];
        }
    },
    setup() {
        return {
        }
    },

    data() {
        return {
            tokenChecked: false,
            errorCode: null,
        };
    },
    mounted() {
    },
    methods: {
    },
    computed: {
        ...mapState({
            tokens: state => state.configuration.tokens,
        })
    },
    components: {}
}
</script>

<template>
    <div class="d-flex flex-column min-vh-100 justify-content-center align-items-center h4 text-center">
        <span v-if="!tokenChecked">
            <p v-html="$t('token.token_being_checked_html')"></p>
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </span>
        <p v-if="tokenChecked && errorCode == 'invalid'" v-html="$t('token.error_token_invalid_html')"></p>
        <p v-if="tokenChecked && errorCode == 'expired'" v-html="$t('token.error_token_expired_html')"></p>
        <p v-if="tokenChecked && errorCode == 'unknown'" v-html="$t('token.error_token_unknown_html')"></p>
    </div>
</template>

<style></style>