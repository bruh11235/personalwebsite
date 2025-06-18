const text = document.querySelector("p");
const currentDate = new Date();
const enrollDate = new Date('2023-09-01T00:00:00Z');
const ordinals = ["first", "second", "third", "fourth", "fifth"];

// Calculate date difference in years
let diff = Math.abs(currentDate - enrollDate)
diff = Math.floor(diff / (1000 * 60 * 60 * 24 * 365.25))

if (diff >= 5) {
    text.textContent = "I have completed my undergrad in computer science at the University of Toronto.";
} else {
    text.textContent = text.textContent.replace("#", ordinals[diff]);
}