
const itemForm = document.getElementById("itemForm");
const contactForm = document.getElementById("contactForm");
const itemList = document.getElementById("itemList");
const searchBox = document.getElementById("searchBox");
const filterType = document.getElementById("filterType");

let items = [];

function makeElement(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    element.textContent = text;
    return element;
}

function updateStats() {
    document.getElementById("lostCount").textContent =
        items.filter(item => item.type === "Lost" && item.status !== "Returned").length;

    document.getElementById("foundCount").textContent =
        items.filter(item => item.type === "Found" && item.status !== "Returned").length;

    document.getElementById("returnedCount").textContent =
        items.filter(item => item.status === "Returned").length;
}

function renderItems() {
    itemList.replaceChildren();

    const search = searchBox.value.trim().toLowerCase();
    const filter = filterType.value;

    const visibleItems = items.filter(item => {
        const matchesSearch =
            item.name.toLowerCase().includes(search) ||
            item.location.toLowerCase().includes(search) ||
            item.category.toLowerCase().includes(search);

        const matchesFilter =
            filter === "All" ||
            (filter === "Returned"
                ? item.status === "Returned"
                : item.type === filter && item.status !== "Returned");

        return matchesSearch && matchesFilter;
    });

    if (visibleItems.length === 0) {
        itemList.appendChild(
            makeElement("p", "empty",
                "No reports found. Try another search or add a report.")
        );
    }

    visibleItems.forEach(item => {
        const card = makeElement("article", "item-card", "");

        const status = makeElement(
            "span",
            "tag " + (item.status === "Returned"
                ? "returned"
                : item.type === "Found" ? "found" : ""),
            item.status
        );

        card.appendChild(status);
        card.appendChild(makeElement("h3", "", item.name));
        card.appendChild(makeElement("p", "", "Category: " + item.category));
        card.appendChild(makeElement("p", "", "Location: " + item.location));
        card.appendChild(makeElement("p", "", "Date: " + item.date));
        card.appendChild(makeElement("p", "", "Description: " + item.description));
        card.appendChild(makeElement("p", "", "Reported by: " + item.reporter));

        const actions = makeElement("div", "item-actions", "");

        if (item.status !== "Returned") {
            const returnedButton = makeElement("button", "small-btn", "Mark Returned");
            returnedButton.type = "button";
            returnedButton.dataset.action = "returned";
            returnedButton.dataset.id = String(item.id);
            actions.appendChild(returnedButton);
        }

        const deleteButton = makeElement("button", "small-btn", "Delete");
        deleteButton.type = "button";
        deleteButton.dataset.action = "delete";
        deleteButton.dataset.id = String(item.id);
        actions.appendChild(deleteButton);

        card.appendChild(actions);
        itemList.appendChild(card);
    });

    updateStats();
}

async function loadItems() {
    try {
        const response = await fetch("/api/items");
        if (!response.ok) throw new Error("Could not load reports");

        const data = await response.json();
        items = data.items || [];
        renderItems();
    } catch (error) {
        console.error(error);
        itemList.textContent = "Unable to load reports. Please refresh the page.";
    }
}

itemForm.addEventListener("submit", async function(event) {
    event.preventDefault();

    const data = new FormData(itemForm);

    const item = {
        name: data.get("itemName").trim(),
        type: data.get("itemType"),
        category: data.get("category"),
        location: data.get("location").trim(),
        date: data.get("itemDate"),
        reporter: data.get("reporter").trim(),
        description: data.get("description").trim()
    };

    if (!item.name || !item.location || !item.reporter || !item.description) {
        document.getElementById("itemMessage").textContent =
            "Please complete all required fields.";
        return;
    }

    try {
        const response = await fetch("/api/items", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(item)
        });

        if (!response.ok) throw new Error(await response.text());

        itemForm.reset();
        document.getElementById("itemMessage").textContent =
            "Report added successfully!";
        await loadItems();
    } catch (error) {
        console.error(error);
        document.getElementById("itemMessage").textContent =
            "Unable to save report. Please try again.";
    }
});

itemList.addEventListener("click", async function(event) {
    const button = event.target.closest("button[data-action]");
    if (!button) return;

    const id = Number(button.dataset.id);
    const action = button.dataset.action;

    try {
        let response;

        if (action === "returned") {
            response = await fetch(`/api/items/${id}/returned`, {
                method: "POST"
            });
        } else if (action === "delete") {
            response = await fetch(`/api/items/${id}`, {
                method: "DELETE"
            });
        }

        if (!response || !response.ok) {
            throw new Error("Action failed");
        }

        await loadItems();
    } catch (error) {
        console.error(error);
        alert("Unable to update the report. Please try again.");
    }
});

searchBox.addEventListener("input", renderItems);
filterType.addEventListener("change", renderItems);

contactForm.addEventListener("submit", async function(event) {
    event.preventDefault();

    const data = new FormData(contactForm);

    const message = {
        name: data.get("contactName").trim(),
        email: data.get("email").trim(),
        subject: data.get("subject").trim(),
        message: data.get("contactMessage").trim()
    };

    if (!message.name || !message.email ||
        !message.subject || !message.message) {
        document.getElementById("contactStatus").textContent =
            "Please complete all fields.";
        return;
    }

    try {
        const response = await fetch("/api/contact", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(message)
        });

        if (!response.ok) throw new Error(await response.text());

        contactForm.reset();
        document.getElementById("contactStatus").textContent =
            "Message saved successfully!";
    } catch (error) {
        console.error(error);
        document.getElementById("contactStatus").textContent =
            "Unable to send message. Please try again.";
    }
});

loadItems();
