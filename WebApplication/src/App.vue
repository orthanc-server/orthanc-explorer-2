<script>
export default {
    async created() {
        console.log("Creating App...");
        await this.$store.dispatch('configuration/load');
        await this.$store.dispatch('studies/initialLoad');
        await this.$store.dispatch('labels/refresh');
        console.log("App created");
    },

}
</script>

<template>
    <div class="full-page">
        <div class="nav-side-layout">
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