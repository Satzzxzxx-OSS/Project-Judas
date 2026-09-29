# Development fixture wiring failure

The first opt-in editor hook ran after ImGui::Render and then called HandleRequests, whose path-popup drawing requires an active ImGui frame. The preserved GDB trace identifies ImGui::BeginPopupModal. This was a new automation scheduling error, not an engine physics or lifecycle failure. The hook now runs immediately after the ordinary HandleRequests call and before ImGui::Render. No simulation, resource, or authoring method was replaced. The subsequent development-3 run passed all 49 checks through the actual editor loop.
