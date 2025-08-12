<script>
import api from "./orthancApi"
import Inbox from "./components/Inbox.vue";

export default {
    async created() {
        console.log("Creating Inbox App...");
        await this.$store.dispatch('configuration/load');
        console.log("Inbox App created");
    },
    components: { Inbox }
}
</script>

<template>
    <div class="full-page">
        <Inbox>
        </Inbox>
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