import { useEffect, type ReactNode } from "react";

interface ModalProps {
  onClose: () => void;
  title?: string;
  children: ReactNode;
}

export function Modal({ onClose, title, children }: ModalProps) {
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") onClose();
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [onClose]);

  return (
    <div className="modal-backdrop" onClick={onClose}>
      <div
        className="modal"
        role="dialog"
        aria-modal="true"
        onClick={(e) => e.stopPropagation()}
      >
        <header className="modal-bar">
          {title && <h2 className="modal-title">{title}</h2>}
          <button
            type="button"
            className="modal-close"
            onClick={onClose}
            aria-label="Close viewer"
          >
            ✕
          </button>
        </header>
        {children}
      </div>
    </div>
  );
}
