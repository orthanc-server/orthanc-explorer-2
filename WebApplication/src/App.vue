<script>
export default {
    async created() {
        console.log("Creating App...");
        await this.$store.dispatch('configuration/load');
        await this.$store.dispatch('studies/initialLoad');
        console.log("App created");
    },

}
</script>

<template>
    <div class="full-page">
        <div class="nav-side-menu">
            <router-view name="SideBarView"></router-view>
        </div>
        <div class="content">
            <router-view name="ContentView"></router-view>
        </div>
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