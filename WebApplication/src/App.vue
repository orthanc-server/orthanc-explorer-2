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
    data() {
        return {
            sidebarOpen: false,
        };
    },
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
    watch: {
        $route() {
            // Auto-close the sidebar on mobile when navigating
            this.sidebarOpen = false;
        },
    },
    methods: {
        toggleSidebar() {
            this.sidebarOpen = !this.sidebarOpen;
        },
        closeSidebar() {
            this.sidebarOpen = false;
        },
    },
}
</script>

<template>
    <div class="full-page">
        <!-- Mobile burger toggle button -->
        <button id="mobile-sidebar-toggle" class="mobile-sidebar-toggle" @click="toggleSidebar"
            :aria-expanded="sidebarOpen" aria-label="Toggle navigation menu">
            <span class="burger-icon">
                <span></span>
                <span></span>
                <span></span>
            </span>
        </button>

        <!-- Backdrop overlay for mobile -->
        <div class="sidebar-overlay" :class="{ 'sidebar-overlay--active': sidebarOpen }" @click="closeSidebar"></div>

        <div class="nav-side-layout" :class="{ 'nav-side-layout--open': sidebarOpen }">
            <router-view name="SideBarView" @close-sidebar="closeSidebar"></router-view>
        </div>
        <div class="content">
            <router-view name="ContentView"></router-view>
        </div>
    </div>
</template>

<style lang="scss">

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
    transition: transform 0.3s ease;
    z-index: 1040;

    @include mobile-any {
        transform: translateX(calc(-1 * var(--nav-bar-width, 260px)));
    }
}

.nav-side-layout .toggle-btn {
    display: none;
}

/* ───── Mobile burger button ───── */
.mobile-sidebar-toggle {
    /* Hidden on desktop */
    display: none;

    /* Show on mobile */
    @include mobile-any {
        display: flex;
        align-items: center;
        justify-content: center;
        position: fixed;
        top: 6px;
        left: 8px;
        z-index: 1050;
        background-color: var(--study-even-bg-color, #212529);
        border: none;
        border-radius: 6px;
        width: 36px;
        height: 32px;
        padding: 0;
        cursor: pointer;
        box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4);
        transition: background-color 0.2s ease;
    }
}

.mobile-sidebar-toggle:hover {
    background-color: var(--nav-side-selected-bg-color, #4f5b69);
}

.burger-icon {
    display: flex;
    flex-direction: column;
    gap: 4px;
    width: 18px;
}

.burger-icon span {
    display: block;
    height: 2px;
    width: 100%;
    background-color: var(--nav-side-color, #ffffff);
    border-radius: 2px;
    transition: transform 0.25s ease, opacity 0.25s ease;
}

/* Animate burger → X when open */
.mobile-sidebar-toggle[aria-expanded="true"] .burger-icon span:nth-child(1) {
    transform: translateY(5px) rotate(45deg);
}

.mobile-sidebar-toggle[aria-expanded="true"] .burger-icon span:nth-child(2) {
    opacity: 0;
}

.mobile-sidebar-toggle[aria-expanded="true"] .burger-icon span:nth-child(3) {
    transform: translateY(-7px) rotate(-45deg);
}

/* ───── Overlay backdrop ───── */
.sidebar-overlay {
    display: none;

    /* Show the overlay (pointer-events still none until --active is added) */
    @include mobile-any {
        display: block;
        position: fixed;
        inset: 0;
        background: rgba(0, 0, 0, 0.5);
        z-index: 1035;
        opacity: 0;
        pointer-events: none;
        /* invisible overlay must NOT intercept touch/clicks */
        transition: opacity 0.3s ease;
    }
}

.sidebar-overlay--active {
    opacity: 1;
    pointer-events: auto;
    /* only capture events when the sidebar is actually open */
}


@include mobile-any {
    /* Slide sidebar off-screen by default */
    .nav-side-layout--open {
        transform: translateX(0);
    }
}

</style>