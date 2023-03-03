<script>
import api from "./orthancApi"
import TokenLanding from "./components/TokenLanding.vue";

export default {
    async created() {
        console.log("Creating Landing App...");
        // await this.$store.dispatch('configuration/load');
        // await this.$store.dispatch('studies/initialLoad');
        console.log("Landing App created");
    },
    components: { TokenLanding }
}
</script>

<template>
    <div class="full-page">
        <TokenLanding>
        </TokenLanding>
    </div>
</template>

<style>
#app {
    font-family: Avenir, Helvetica, Arial, sans-serif;
    -webkit-font-smoothing: antialiased;
    -moz-osx-font-smoothing: grayscale;
    text-align: center;
    color: #2c3e50;
    margin-top: 10px;
}

.nav-side-layout {
    overflow: auto;
    position: fixed;
    top: 0px;
    width: var(--nav-bar-width);
    height: 100%;
    background-color: var(--nav-side-bg-color);
    color: var(--nav-side-color);
}

.nav-side-layout .toggle-btn {
    display: none;
}

</style>