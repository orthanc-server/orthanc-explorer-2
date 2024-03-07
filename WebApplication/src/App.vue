<script>
function applyBootStrapTheme() {
    // hack to switch the theme: get the value from our custom css
    let bootstrapTheme = getComputedStyle(document.documentElement).getPropertyValue('--bootstrap-theme');
    console.log("-------------- Applying Bootstrap theme ...", bootstrapTheme);
    if (bootstrapTheme) {
        // and set it to the 'html' element
        document.documentElement.setAttribute('data-bs-theme', bootstrapTheme);
    } else {
        console.warn("-------------- Applying Bootstrap theme not defined yet, retrying ...");
        setTimeout(applyBootStrapTheme, 100);
    }
}

export default {
    async created() {
        console.log("Creating App...");

        {// Load the CSS dynamically since it can be a custom css
            console.log("Loading the defaults + custom CSS ...");

            let link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            link.href = 'customizable/custom.css';
    
            document.getElementsByTagName('HEAD')[0].appendChild(link);

            setTimeout(applyBootStrapTheme, 0);
        }
        // {
        //     console.log("Loading the custom CSS ...");

        //     let link = document.createElement('link');
        //     link.rel = 'stylesheet';
        //     link.type = 'text/css';
        //     link.href = 'customizable/custom.css';
    
        //     document.getElementsByTagName('HEAD')[0].appendChild(link);
        // }

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