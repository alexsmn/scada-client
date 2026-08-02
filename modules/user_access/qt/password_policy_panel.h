#pragma once

#include "user_access/password_policy.h"

#include <QWidget>

#include <optional>

class QLabel;
class QVBoxLayout;

// The Password policy view — the "Password policy" section of the
// Administration explorer (users-admin.html).
//
// It shows what the SERVER publishes (OPC UA Part 18 §5.2.2): the length
// range, which character classes are required, and the deployment's own
// human-readable statement of the rule. Nothing here is a client-side rule —
// an operator reading this is reading the policy their password will actually
// be validated against.
//
// Read-only. The policy is server configuration, not something a client edits.
class PasswordPolicyPanel : public QWidget {
  Q_OBJECT

 public:
  explicit PasswordPolicyPanel(QWidget* parent = nullptr);
  ~PasswordPolicyPanel() override;

  // `policy` is nullopt when the UserManagement object could not be read,
  // which is shown as such: an unconstrained server and an unreadable one must
  // not look alike, because the difference decides whether a rejected password
  // is the operator's fault.
  void ShowPolicy(const std::optional<PasswordPolicy>& policy);

 private:
  QWidget* BuildHeader();

  QLabel* title_ = nullptr;
  QLabel* length_ = nullptr;
  QLabel* restrictions_ = nullptr;
  QVBoxLayout* requirements_ = nullptr;
};

// Builds a PasswordPolicyPanel under the reshell UX theme; nullptr in the
// legacy look. Ownership transfers to the caller.
PasswordPolicyPanel* MakePasswordPolicyPanel();
