export default {
    isValidEmailDestination(destination) {
        if (!destination) {
            return false;
        }
        // Regex to validate individual email addresses
        const emailRegex = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/;

        // Split the string into individual email addresses
        const emails = destination.split(/[;,\s]+/).filter(email => email);

        // Check if all entries are valid email addresses
        return emails.every(email => emailRegex.test(email));
   }
}