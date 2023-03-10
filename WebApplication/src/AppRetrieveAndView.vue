<script>
import api from "./orthancApi"
import RetrieveAndView from "./components/RetrieveAndView.vue";

export default {
    async created() {
        console.log("Creating RetrieveAndView App...");
        await this.$store.dispatch('configuration/load');
        // await this.$store.dispatch('studies/initialLoad');
        console.log("RetrieveAndView App created");
    },
    components: { RetrieveAndView }
}
</script>

<template>
    <div class="full-page">
        <RetrieveAndView>
        </RetrieveAndView>
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

</style>