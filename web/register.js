document.addEventListener("DOMContentLoaded", () => {
  const form = document.getElementById("myRegister");

  // ถ้าอยู่หน้า register
  if (form) {
    form.addEventListener("submit", (event) => {
      event.preventDefault(); // ป้องกัน reload หน้า

      // เก็บข้อมูลทั้งหมดใน object
      const data = {
        firstname: form.firstname.value,
        lastname: form.lastname.value,
        gender: form.gender.value,
        bday: form.bday.value,
        bloodgroup: form.bloodgroup.value,
        disease: form.disease.value,
        allergy: form.allergy.value,
        medication: form.medication.value,
        emergencyname: form.emergencyname.value,
        emergencyphone: form.emergencyphone.value
      };

      // บันทึกข้อมูลใน localStorage
      localStorage.setItem("profileData", JSON.stringify(data));

      // เปิดหน้าโปรไฟล์
      window.location.href = "profile.html";
    });
  }

  // ถ้าอยู่หน้า profile.html
  const profileDiv = document.getElementById("profileData");
  if (profileDiv) {
    const data = JSON.parse(localStorage.getItem("profileData"));

    if (data) {
      profileDiv.innerHTML = `
        <div class="info"><span>Name:</span> ${data.firstname} ${data.lastname}</div>
        <div class="info"><span>Gender:</span> ${data.gender}</div>
        <div class="info"><span>Birthday:</span> ${data.bday}</div>
        <div class="info"><span>Blood Group:</span> ${data.bloodgroup}</div>
        <div class="info"><span>Underlying Diseases:</span> ${data.disease || '-'}</div>
        <div class="info"><span>Allergies:</span> ${data.allergy || '-'}</div>
        <div class="info"><span>Current Medications:</span> ${data.medication || '-'}</div>
        <div class="info"><span>Emergency Contact:</span> ${data.emergencyname}</div>
        <div class="info"><span>Emergency Phone:</span> ${data.emergencyphone}</div>
      `;
    } else {
      profileDiv.innerHTML = `<p>No profile data found. Please <a href='register.html'>register</a> first.</p>`;
    }
  }
});
